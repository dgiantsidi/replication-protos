#include "common.h"
#include "msg.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

static constexpr int kReq = 1;
static constexpr int numa_node = 0;

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(window_size, 1, "Outstanding requests per client");
DEFINE_uint64(req_size, 64, "Size of request message in bytes");
DEFINE_uint64(resp_size, 32, "Size of response message in bytes");
DEFINE_uint64(process_id, 0, "Process id");
DEFINE_uint64(instance_id, 0, "Instance id (this is to properly set the RPCs");

using rpc_handle = erpc::Rpc<erpc::CTransport>;

struct rpc_buffs {
  erpc::MsgBuffer req;
  erpc::MsgBuffer resp;

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

  ~rpc_buffs() {}
};

class app_context {
public:
  rpc_handle *rpc = nullptr;
  int node_id = 0, thread_id = 0;
  int instance_id = 0; /* instance id used to create the rpcs */
  msg_manager *batcher = nullptr;

  uint64_t enqueued_msgs_cnt = 0;
  /* map of <node_id, session_num> that maintains connections */
  std::unordered_map<int, int> connections;

  /* delete copy/move constructors/assignments
     app_context(const app_context &) = delete;
     app_context(app_context &&) = delete;
     app_context &operator=(const app_context &) = delete;
     app_context &operator=(app_context &&) = delete;
     */
  ~app_context() {
    fmt::print("{} ..\n", __func__);
    delete rpc;
  }
};

static void cont_func(void *context, void *t) {
  auto *ctx = static_cast<app_context *>(context);
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr \n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  auto *tag = static_cast<rpc_buffs *>(t);
  ctx->rpc->free_msg_buffer(tag->req);
  ctx->rpc->free_msg_buffer(tag->resp);

  delete tag;
}
void send_req(int idx, int dest_id, app_context *ctx) {
  // construct message
  auto msg_buf = std::make_unique<msg>();
  msg_buf->hdr.src_node = ctx->node_id;
  msg_buf->hdr.dest_node = dest_id;
  msg_buf->hdr.seq_idx = idx;
  msg_buf->hdr.msg_size = kMsgSize;
  if (!ctx->batcher->enqueue_req(reinterpret_cast<uint8_t *>(msg_buf.get()),
                                 sizeof(msg))) {
    // needs to send
    rpc_buffs *buffs = new rpc_buffs();
    buffs->alloc_req_buf(sizeof(msg) * msg_manager::batch_count, ctx->rpc);
    buffs->alloc_resp_buf(kMsgSize, ctx->rpc);

    ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
             sizeof(msg) * msg_manager::batch_count);
    // enqueue_req
    ctx->rpc->enqueue_request(ctx->connections[dest_id], kReq, &buffs->req,
                              &buffs->resp, cont_func,
                              reinterpret_cast<void *>(buffs));
    ctx->batcher->empty_buff();
    ctx->batcher->enqueue_req(reinterpret_cast<uint8_t *>(msg_buf.get()),
                              sizeof(msg));
    //    fmt::print("{} idx={}\n", __func__, idx);
    ctx->enqueued_msgs_cnt++;
  }
}

void flash_batcher(int dest_id, app_context *ctx) {
  // needs to send
  rpc_buffs *buffs = new rpc_buffs();
  buffs->alloc_req_buf(sizeof(msg) * ctx->batcher->cur_idx, ctx->rpc);
  buffs->alloc_resp_buf(kMsgSize, ctx->rpc);

  ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
           sizeof(msg) * ctx->batcher->cur_idx);
  // enqueue_req
  ctx->rpc->enqueue_request(ctx->connections[dest_id], kReq, &buffs->req,
                            &buffs->resp, cont_func,
                            reinterpret_cast<void *>(buffs));
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
  fmt::print("[{}] connected to uri={} w/ remote RPC_id={} session_num={}\n",
             __func__, uri, ctx->instance_id, session_num);
  ctx->connections.insert({rem_node_id, session_num});
  ctx->rpc->run_event_loop(3000);
}

enum chain_replication {
  head = 0,
  tail = 1,
  middle // FIXME: @dimitra that is only for testing
};

void head_func(app_context *ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  create_session(uri, 1 /* node 1 */, ctx);
  ctx->rpc->run_event_loop(100);
  for (auto i = 0ULL; i < 40e6; i++) {
    send_req(i, 1, ctx);
    poll(ctx);
  }
  flash_batcher(1, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  for (;;)
    ctx->rpc->run_event_loop_once();
}

void tail_func(app_context *ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  create_session(uri, 0 /* node 0 */, ctx);
  ctx->rpc->run_event_loop(100);
  for (auto i = 0ULL; i < 40e6; i++) {
    send_req(i, 0, ctx);
    poll(ctx);
  }
  flash_batcher(0, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  for (;;)
    ctx->rpc->run_event_loop_once();
}
void middle_func(app_context *ctx, const std::string &uri1,
                 const std::string &uri2) {}

void proto_func(size_t thread_id, erpc::Nexus *nexus) {
  fmt::print("[{}]\tthread_id={} starts\n", __PRETTY_FUNCTION__, thread_id);
  app_context *ctx = new app_context();
  ctx->batcher = new msg_manager();
  ctx->instance_id = FLAGS_instance_id;
  ctx->node_id = FLAGS_process_id;
  ctx->rpc = new erpc::Rpc<erpc::CTransport>(nexus, static_cast<void *>(ctx),
                                             ctx->instance_id, sm_handler, 0);

  /* give some time before we start */
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(5000ms);
  /* we can start */

  if (FLAGS_process_id == chain_replication::head)
    head_func(ctx, std::string{kmartha});
  else if (FLAGS_process_id == chain_replication::middle)
    middle_func(ctx, std::string{}, std::string{});
  else if (FLAGS_process_id == ::chain_replication::tail)
    tail_func(ctx, std::string{kdonna});
  fmt::print("[{}]\tthread_id={} exits.\n", __PRETTY_FUNCTION__, thread_id);
}

void ctrl_c_handler(int) {
  fmt::print("{} ctrl-c .. exiting\n", __PRETTY_FUNCTION__);
  exit(1);
}

void req_handler(erpc::ReqHandle *req_handle,
                 void *context /*TODO: appcontext */) {
  static uint64_t count = 0;
  // construct message
  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);
  auto batched_msg = msg_manager::deserialize(
      recv_data, req_handle->get_req_msgbuf()->get_data_size());

  if (count % 25000 == 0) {
    // print batched
    fmt::print("{} count={}\n", __func__, count);
    msg_manager::print_batched(std::move(batched_msg),
                               req_handle->get_req_msgbuf()->get_data_size());
#if 0
		fmt::print("[{}]\tmsg_count={}\tidx={}\tnode_id={}\tdest_node={}\n",
				__PRETTY_FUNCTION__, count, msg_buf->hdr.seq_idx,
				msg_buf->hdr.src_node, msg_buf->hdr.dest_node);
#endif
  }
  if (count == 40e6 / 16 - 1)
    fmt::print("{} final count={}\n", __func__, count);
  count++;

  /* enqueue ack */

  app_context *ctx = reinterpret_cast<app_context *>(context);
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
  ctx->rpc->resize_msg_buffer(&resp, kMsgSize);
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
    server_uri = kmartha + ":" + std::to_string(kUDPPort);
  }
  erpc::Nexus nexus(server_uri, 0, 0);
  nexus.register_req_func(kReq, req_handler);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(std::thread(proto_func, i, &nexus));
    erpc::bind_to_core(threads[i], numa_node, i);
  }

  for (auto &thread : threads)
    thread.join();

  return 0;
}
