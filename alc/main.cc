#include "app_context.h"
#include "common.h"
#include "msg.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(process_id, 0, "Process id");
DEFINE_uint64(instance_id, 0,
              "Instance id (this is to properly set the RPCs when using "
              "multiple instances/threads)");
DEFINE_uint64(reqs_num, 200e6, "Number of reqs");

enum alc { sender = 0 };
class app_context;
void send_cmt(int cmt_idx, const std::vector<int> &dest_ids, app_context *ctx);

#if 0
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
  int cns_rd = response->cns_rd;
  int sender = response->hdr.src_node;

  ctx->rpc->free_msg_buffer(tag->req);
  ctx->rpc->free_msg_buffer(tag->resp);

  ctx->metadata->commit_completed_rds(tot_nodes);
  delete tag;
  if ((count % PRINT_BATCH) == 0) {
    fmt::print("[{}] ack-ed {} cmt_reqs\n", __func__, count);
  }
  count++;
}

static void cont_func_send(void *context, void *t) {
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
  int cns_rd = response->cns_rd;
  int sender = response->hdr.src_node;
#if 0
  fmt::print("[{}] ack-ed prp-req for idx={} (from node={})\n", __func__, cns_rd,
             sender);
#endif
  ctx->metadata->proto_meta[cns_rd]->nb_acks++;
  bool can_cmt = ctx->metadata->can_commit(tot_nodes);
  delete tag;
  count++;
  if (can_cmt)
    send_cmt(cns_rd, ctx->metadata->followers, ctx);
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
#endif

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
      buffs->alloc_req_buf(msg_manager::batch_sz, ctx->rpc);
      buffs->alloc_resp_buf(sizeof(cmt_msg), ctx->rpc);

      ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
               sizeof(msg) * msg_manager::batch_count);
      // enqueue_req
      ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqSend,
                                &buffs->req, &buffs->resp, cont_func_send,
                                reinterpret_cast<void *>(buffs));
    }
    auto ptr =
        std::make_unique<uint8_t[]>(sizeof(msg) * msg_manager::batch_count);
    ::memcpy(ptr.get(), ctx->batcher->serialize_batch(),
             sizeof(msg) * msg_manager::batch_count);
    ctx->metadata->add_recv_req(ctx->node_id, std::move(ptr),
                                ctx->batcher->count);
    ctx->metadata->cur_rd = ctx->batcher->count;
#ifdef PRINT_DEBUG
    fmt::print("[{}] send_batch with rd={}\n", __PRETTY_FUNCTION__,
               ctx->batcher->count);
#endif
    // ctx->metadata->proto_meta[ctx->batcher->count]->nb_acks++;
    /* empty_buff() increases the cound/consensus rd id */
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
    buffs->alloc_req_buf(sizeof(msg) * ctx->batcher->cur_idx +
                             sizeof(msg_manager::consensus_round),
                         ctx->rpc);
    buffs->alloc_resp_buf(kMsgSize, ctx->rpc);

    ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
             sizeof(msg) * ctx->batcher->cur_idx);
    // enqueue_req
    ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqSend, &buffs->req,
                              &buffs->resp, cont_func_send,
                              reinterpret_cast<void *>(buffs));
  }
  auto ptr =
      std::make_unique<uint8_t[]>(sizeof(msg) * msg_manager::batch_count);
  ::memcpy(ptr.get(), ctx->batcher->serialize_batch(),
           sizeof(msg) * msg_manager::batch_count);
  ctx->metadata->add_recv_req(ctx->node_id, std::move(ptr),
                              ctx->batcher->count);
  fmt::print("[{}] send_batch with rd={}\n", __PRETTY_FUNCTION__,
             ctx->batcher->count);
  ctx->metadata->cur_rd = ctx->batcher->count;
  ctx->metadata->proto_meta[ctx->batcher->count]->nb_acks++;
  ctx->batcher->empty_buff();
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
  ctx->rpc->run_event_loop(1000);
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

  using uri_tuple = std::tuple<int, std::string>;
  if (FLAGS_process_id == kdonna_id) {
    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{kmartha_id, std::string{kmartha}},
        uri_tuple{krose_id, std::string{krose}}};
    leader_func(ctx, uris);
  } else if (FLAGS_process_id == krose_id) {
    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{kmartha_id, std::string{kmartha}},
        uri_tuple{kdonna_id, std::string{kdonna}}};
    leader_func(ctx, uris);
  } else if (FLAGS_process_id == kmartha_id) {
    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{kdonna_id, std::string{kdonna}},
        uri_tuple{krose_id, std::string{krose}}};
    leader_func(ctx, uris);
  }

  fmt::print("[{}]\tthread_id={} exits.\n", __PRETTY_FUNCTION__, thread_id);
}

void ctrl_c_handler(int) {
  fmt::print("{} ctrl-c .. exiting\n", __PRETTY_FUNCTION__);
  exit(1);
}

void req_handler_send(erpc::ReqHandle *req_handle,
                      void *context /* app_context */) {
  static uint64_t count = 0;
  app_context *ctx = reinterpret_cast<app_context *>(context);

  // deserialize the message-req
  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);
  auto batched_msg = msg_manager::deserialize(
      recv_data, req_handle->get_req_msgbuf()->get_data_size());
  auto cns_rd = ctx->batcher->get_consensus_round(batched_msg.get());
  auto ptr = std::make_unique<uint8_t[]>(
      req_handle->get_req_msgbuf()->get_data_size());
  ::memcpy(ptr.get(), batched_msg.get(),
           req_handle->get_req_msgbuf()->get_data_size());

  //  fmt::print("{} ", __func__);
  ctx->metadata->add_recv_req(0, std::move(ptr),
                              cns_rd); // TODO: fix the 0->sender-id
  ctx->metadata->proto_meta[cns_rd]->nb_acks++;

#ifdef PRINT_DEBUG
  if (count % PRINT_BATCH == 0) {
    fmt::print("[{}] proto_meta[{}]->nb_acks={}\n", __func__, cns_rd,
               ctx->metadata->proto_meta[cns_rd]->nb_acks);
  }
#endif
  if (count == FLAGS_reqs_num / msg_manager::batch_count - 1)
    fmt::print("{} final count={}\n", __func__, count);
  count++;

  /* enqueue ack */

  auto ack = std::make_unique<cmt_msg>();
  ack->cns_rd = ctx->batcher->get_consensus_round(batched_msg.get());
  // ack->e_idx = idxs[1];
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

  bool can_cmt = ctx->metadata->can_commit(tot_acks, cns_rd);
  if (can_cmt) {
#ifdef PRINT_DEBUG
    fmt::print("[{}] cns_rd={} is eligible for commit\n", __PRETTY_FUNCTION__,
               cns_rd);
#endif
    // ctx->metadata->proto_meta[cns_rd]->nb_cmts++;
    /* all nodes have received all messages so sent commit-msg for this round */
    send_cmt(cns_rd, ctx->metadata->followers, ctx);
  }
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
#ifdef PRINT_DEBUG
    fmt::print("{} count={}\n", __func__, count);
#endif
  }

  count++;

  /* enqueue ack */

  app_context *ctx = reinterpret_cast<app_context *>(context);
#ifdef PRINT_DEBUG
  fmt::print("[{}] cmt_idx={} from node={}\n", __PRETTY_FUNCTION__,
             msg_data->hdr.seq_idx, msg_data->hdr.src_node);
#endif
  ctx->metadata->proto_meta[msg_data->hdr.seq_idx]->nb_cmts++;
  auto ack = std::make_unique<cmt_msg>();
  ack->cns_rd = msg_data->hdr.seq_idx;
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
  ctx->metadata->commit_completed_rds(tot_acks);
}

int main(int args, char *argv[]) {
  signal(SIGINT, ctrl_c_handler);
  gflags::ParseCommandLineFlags(&args, &argv, true);

  size_t num_threads = FLAGS_process_id == 0 ? FLAGS_num_server_threads
                                             : FLAGS_num_client_threads;

  std::string server_uri;
  if (FLAGS_process_id == kdonna_id) {
    server_uri = kdonna + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == krose_id) {
    server_uri = krose + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == kmartha_id) {
    server_uri = kmartha + ":" + std::to_string(kUDPPort);
  } else {
    fmt::print("[{}] not valid process_id={}\n", __func__, FLAGS_process_id);
  }
  erpc::Nexus nexus(server_uri, 0, 0);
  nexus.register_req_func(kReqSend, req_handler_send);
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
