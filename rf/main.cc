#include "common.h"
#include "msg.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

static constexpr int kReqPropose = 1;
static constexpr int kReqCommit = 2;
static constexpr int PRINT_BATCH = 50000;
static constexpr int numa_node = 0;

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(process_id, 0, "Process id");
DEFINE_uint64(instance_id, 0,
              "Instance id (this is to properly set the RPCs when using "
              "multiple instances/threads)");
DEFINE_uint64(reqs_num, 200e6, "Number of reqs");

using rpc_handle = erpc::Rpc<erpc::CTransport>;
enum raft { leader = 0, follower = 1 };
class app_context;
void send_cmt(int cmt_idx, const std::vector<int> &dest_ids, app_context *ctx);

struct rpc_buffs {
  erpc::MsgBuffer req;
  erpc::MsgBuffer resp;
  rpc_handle *rpc = nullptr;
  explicit rpc_buffs(rpc_handle *r) : rpc(r){};
  rpc_buffs() = delete;

  void alloc_req_buf(size_t sz, rpc_handle *rpc) {
    req = rpc->alloc_msg_buffer(sz);
    while (req.buf == nullptr) {
      fmt::print("[{}] failed to allocate memory\n", __func__);
      rpc->run_event_loop(100);
      req = rpc->alloc_msg_buffer(sz);
    }
  }

  void alloc_resp_buf(size_t sz, rpc_handle *rpc) {
    resp = rpc->alloc_msg_buffer(sz);
    while (resp.buf == nullptr) {
      fmt::print("[{}] failed to allocate memory\n", __func__);
      rpc->run_event_loop(100);
      resp = rpc->alloc_msg_buffer(sz);
    }
  }

  ~rpc_buffs() {
    // rpc->free_msg_buffer(req);
    // rpc->free_msg_buffer(resp);
  }
};

class app_context {
public:
  struct protocol_metadata {
    int leader = raft::follower;
    bool is_leader() { return leader == raft::leader; }
    int cmt_idx = 0, prp_idx = 0;
    std::vector<int> followers;

    using req_id = int;
    using nb_acks = int;
    std::unordered_map<req_id, nb_acks> prp_acks;
    std::unordered_map<req_id, nb_acks> cmt_acks;

    bool update_prp_acks(int idx, int src_node) {
      if (prp_acks.find(idx) == prp_acks.end()) {
        prp_acks.insert({idx, 1});
        return false;
      } else {
        prp_acks.erase(idx);
        return true;
      }
    }
    bool update_cmt_acks(int idx, int src_node) {
      if (cmt_acks.find(idx) == cmt_acks.end()) {
        cmt_acks.insert({idx, 1});
        return false;
      } else {
        prp_acks.erase(idx);
        return true;
      }
    }
  };

  rpc_handle *rpc = nullptr;
  int node_id = 0, thread_id = 0;
  int instance_id = 0; /* instance id used to create the rpcs */
  msg_manager *batcher = nullptr;
  protocol_metadata *metadata = nullptr;

  uint64_t enqueued_msgs_cnt = 0;

  /* map of <node_id, session_num> that maintains connections */
  std::unordered_map<int, int> connections;

  ~app_context() {
    fmt::print("{} ..\n", __func__);
    delete rpc;
  }
};

static void cont_func_cmt(void *context, void *t) {
  static uint64_t count = 0;
  auto *ctx = static_cast<app_context *>(context);
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr \n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  auto *tag = static_cast<rpc_buffs *>(t);
  auto *response = reinterpret_cast<cmt_msg *>(tag->resp.buf);
  int e_idx = response->e_idx;
  int sender = response->hdr.src_node;

  ctx->rpc->free_msg_buffer(tag->req);
  ctx->rpc->free_msg_buffer(tag->resp);

  ctx->metadata->update_cmt_acks(e_idx, sender);
  delete tag;
  if ((count % PRINT_BATCH) == 0) {
    fmt::print("[{}] ack-ed {} prp_reqs\n", __func__, count);
  }
  count++;
}

static void cont_func_prp(void *context, void *t) {
  static uint64_t count = 0;
  auto *ctx = static_cast<app_context *>(context);
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr \n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  auto *tag = static_cast<rpc_buffs *>(t);

  ctx->rpc->free_msg_buffer(tag->req);
  ctx->rpc->free_msg_buffer(tag->resp);

  auto *response = reinterpret_cast<cmt_msg *>(tag->resp.buf);
  if ((count % PRINT_BATCH) == 0) {
    fmt::print("[{}] ack-ed {} prp_reqs\n", __func__, count);
  }
  int e_idx = response->e_idx;
  int sender = response->hdr.src_node;
#ifdef PRINT_DEBUG
  fmt::print("[{}] ack-ed prp-req for idx={} (from node={})\n", __func__, e_idx,
             sender);
#endif
  bool cmt = ctx->metadata->update_prp_acks(e_idx, sender);
  delete tag;
  count++;
  if (cmt)
    send_cmt(e_idx, ctx->metadata->followers, ctx);
}

void send_cmt(int cmt_idx, const std::vector<int> &dest_ids, app_context *ctx) {
  // construct message
  auto msg_buf = std::make_unique<msg>();
  msg_buf->hdr.src_node = ctx->node_id;
  msg_buf->hdr.dest_node = -1; /* broadcast */
  msg_buf->hdr.seq_idx = cmt_idx;
  msg_buf->hdr.msg_size = kMsgSize;
  for (auto &dest_id : dest_ids) {
    // needs to send
    rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
    buffs->alloc_req_buf(sizeof(msg), ctx->rpc);
    buffs->alloc_resp_buf(kMsgSize, ctx->rpc);

    ::memcpy(buffs->req.buf, msg_buf.get(), sizeof(msg));
    // enqueue_req
    ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqCommit,
                              &buffs->req, &buffs->resp, cont_func_cmt,
                              reinterpret_cast<void *>(buffs));
  }
}

void send_req(int idx, const std::vector<int> &dest_ids, app_context *ctx) {
  // construct message
  auto msg_buf = std::make_unique<msg>();
  msg_buf->hdr.src_node = ctx->node_id;
  msg_buf->hdr.dest_node = -1; /* broadcast */
  msg_buf->hdr.seq_idx = idx;
  msg_buf->hdr.msg_size = kMsgSize;
  if (!ctx->batcher->enqueue_req(reinterpret_cast<uint8_t *>(msg_buf.get()),
                                 sizeof(msg))) {
    for (auto &dest_id : dest_ids) {
      // needs to send
      rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
      buffs->alloc_req_buf(sizeof(msg) * msg_manager::batch_count, ctx->rpc);
      buffs->alloc_resp_buf(sizeof(cmt_msg), ctx->rpc);

      ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
               sizeof(msg) * msg_manager::batch_count);
      // enqueue_req
      ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqPropose,
                                &buffs->req, &buffs->resp, cont_func_prp,
                                reinterpret_cast<void *>(buffs));
    }
    ctx->batcher->empty_buff();
    ctx->batcher->enqueue_req(reinterpret_cast<uint8_t *>(msg_buf.get()),
                              sizeof(msg));
    ctx->enqueued_msgs_cnt++;
  }
}

void flash_batcher(const std::vector<int> &dest_ids, app_context *ctx) {
  // needs to send
  for (auto &dest_id : dest_ids) {
    rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
    buffs->alloc_req_buf(sizeof(msg) * ctx->batcher->cur_idx, ctx->rpc);
    buffs->alloc_resp_buf(kMsgSize, ctx->rpc);

    ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
             sizeof(msg) * ctx->batcher->cur_idx);
    // enqueue_req
    ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqPropose,
                              &buffs->req, &buffs->resp, cont_func_prp,
                              reinterpret_cast<void *>(buffs));
  }
}

void poll(app_context *ctx) {
  if (ctx->enqueued_msgs_cnt % 2 == 0) {
    for (auto i = 0; i < 10; i++)
      ctx->rpc->run_event_loop_once();
  }
  if (ctx->enqueued_msgs_cnt % 2 == 10)
    ctx->rpc->run_event_loop(1);
}

[[maybe_unused]] void poll_once(app_context *ctx) {
  ctx->rpc->run_event_loop(100);
}

void create_session(const std::string &uri, int rem_node_id, app_context *ctx) {
  ctx->rpc->retry_connect_on_invalid_rpc_id = true;
  std::string middle_uri = uri + ":" + std::to_string(kUDPPort);
  fmt::print("[{}] uri={}\n", __PRETTY_FUNCTION__, middle_uri);
  int session_num = ctx->rpc->create_session(middle_uri, ctx->instance_id);
  while (!ctx->rpc->is_connected(session_num)) {
    ctx->rpc->run_event_loop_once();
  }
  fmt::print("[{}] connected to uri={} w/ remote RPC_id={} session_num={} "
             "(rem_node_id={})\n",
             __func__, uri, ctx->instance_id, session_num, rem_node_id);
  ctx->connections.insert({rem_node_id, session_num});
  ctx->rpc->run_event_loop(3000);
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> vec) {
  os << "[";
  for (auto &elem : vec) {
    os << " " << elem;
  }
  os << "]\n";
  return os;
}

void leader_func(app_context *ctx,
                 const std::vector<std::tuple<int, std::string>> &uris) {
  std::vector<int> &followers = ctx->metadata->followers;
  for (auto &uri : uris) {
    fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
               __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id,
               std::get<1>(uri));
    create_session(std::get<1>(uri), std::get<0>(uri), ctx);
    followers.push_back(std::get<0>(uri));
  }
  ctx->rpc->run_event_loop(100);
  for (auto i = 0ULL; i < FLAGS_reqs_num; i++) {
    send_req(i, followers, ctx);
    poll(ctx);
  }
  flash_batcher(followers, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  for (;;)
    ctx->rpc->run_event_loop_once();
}

void follower_func(app_context *ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  create_session(uri, raft::leader /* node 0 */, ctx);
  ctx->rpc->run_event_loop(100);
  for (auto i = 0ULL; i < FLAGS_reqs_num; i++) {
    // send_req(i, 0, ctx);
    poll(ctx);
  }
  // flash_batcher(0, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  for (;;)
    ctx->rpc->run_event_loop_once();
}

void proto_func(size_t thread_id, erpc::Nexus *nexus) {
  fmt::print("[{}]\tthread_id={} starts\n", __PRETTY_FUNCTION__, thread_id);
  app_context *ctx = new app_context();
  ctx->batcher = new msg_manager();
  ctx->metadata = new app_context::protocol_metadata();
  ctx->instance_id = thread_id;
  ctx->node_id = FLAGS_process_id;
  ctx->rpc = new rpc_handle(nexus, static_cast<void *>(ctx), ctx->instance_id,
                            sm_handler, 0);

  /* give some time before we start */
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(5000ms);
  /* we can start */

  if (FLAGS_process_id == raft::leader) {
    using uri_tuple = std::tuple<int, std::string>;
#if 1
    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{1, std::string{krose}}, uri_tuple{2, std::string{kmartha}}};
#else

    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{1, std::string{kmartha}}};
#endif
    leader_func(ctx, uris);
  } else {
    follower_func(ctx, std::string{kdonna});
  }

  fmt::print("[{}]\tthread_id={} exits.\n", __PRETTY_FUNCTION__, thread_id);
}

void ctrl_c_handler(int) {
  fmt::print("{} ctrl-c .. exiting\n", __PRETTY_FUNCTION__);
  exit(1);
}

void req_handler_prp(erpc::ReqHandle *req_handle,
                     void *context /* app_context */) {
  static uint64_t count = 0;

  // deserialize the message-req
  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);
  auto batched_msg = msg_manager::deserialize(
      recv_data, req_handle->get_req_msgbuf()->get_data_size());

  std::vector<int> idxs = msg_manager::parse_indexes(
      std::move(batched_msg), req_handle->get_req_msgbuf()->get_data_size());
  if (count % PRINT_BATCH == 0) {
    // print batched
    fmt::print("{} count={}\n", __func__, count);
  }

  if (count == FLAGS_reqs_num / msg_manager::batch_count - 1)
    fmt::print("{} final count={}\n", __func__, count);
  count++;

  /* enqueue ack */

  app_context *ctx = reinterpret_cast<app_context *>(context);
  auto ack = std::make_unique<cmt_msg>();
  ack->e_idx = idxs[1];
  ack->hdr.src_node = ctx->node_id;
  auto &resp = req_handle->pre_resp_msgbuf;
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr \n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  if (ctx->rpc == nullptr) {
    fmt::print("ctx->rpc==nullptr\n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  ctx->rpc->resize_msg_buffer(&resp, sizeof(cmt_msg));
  ::memcpy(resp.buf, ack.get(), sizeof(cmt_msg));
  ctx->rpc->enqueue_response(req_handle, &resp);
}

void req_handler_cmt(erpc::ReqHandle *req_handle,
                     void *context /* app_context */) {
  static uint64_t count = 0;
#ifdef PRINT_DEBUG
  fmt::print("{} count={}\n", __func__, count);
#endif

  // deserialize the message-req
  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);
  auto *msg_data = reinterpret_cast<msg *>(recv_data);
  if (count % PRINT_BATCH == 0) {
    // print batched
    fmt::print("{} count={}\n", __func__, count);
  }

  count++;

  /* enqueue ack */

  app_context *ctx = reinterpret_cast<app_context *>(context);
  auto ack = std::make_unique<cmt_msg>();
  ack->e_idx = msg_data->hdr.seq_idx;
  ack->hdr.src_node = ctx->node_id;
  auto &resp = req_handle->pre_resp_msgbuf;
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr \n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  if (ctx->rpc == nullptr) {
    fmt::print("ctx->rpc==nullptr\n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  ctx->rpc->resize_msg_buffer(&resp, sizeof(cmt_msg));
  ::memcpy(resp.buf, ack.get(), sizeof(cmt_msg));
  ctx->rpc->enqueue_response(req_handle, &resp);
}

int main(int args, char *argv[]) {
  signal(SIGINT, ctrl_c_handler);
  gflags::ParseCommandLineFlags(&args, &argv, true);

  size_t num_threads = FLAGS_process_id == 0 ? FLAGS_num_server_threads
                                             : FLAGS_num_client_threads;

  std::string server_uri;
  if (FLAGS_process_id == 0) {
    server_uri = kdonna + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == 1) {
    server_uri = krose + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == 2) {
    server_uri = kmartha + ":" + std::to_string(kUDPPort);
  } else {
    fmt::print("[{}] not valid process_id={}\n", __func__, FLAGS_process_id);
  }
  erpc::Nexus nexus(server_uri, 0, 0);
  nexus.register_req_func(kReqPropose, req_handler_prp);
  nexus.register_req_func(kReqCommit, req_handler_cmt);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(std::thread(proto_func, i, &nexus));
    erpc::bind_to_core(threads[i], numa_node, i);
  }

  for (auto &thread : threads)
    thread.join();

  return 0;
}
