#include "common.h"
#include "msg.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

static constexpr size_t kAppEvLoopMs = 1000;    // Duration of event loop
static constexpr bool kAppVerbose = false;      // Print debug info on datapath
static constexpr double kAppLatFac = 10.0;      // Precision factor for latency
static constexpr size_t kAppReqType = 1;        // eRPC request type
static constexpr size_t kAppMaxWindowSize = 32; // Max pending reqs per client
static constexpr int numa_node = 0;
static constexpr int kReq = 0;

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(window_size, 1, "Outstanding requests per client");
DEFINE_uint64(req_size, 64, "Size of request message in bytes");
DEFINE_uint64(resp_size, 32, "Size of response message in bytes");
DEFINE_uint64(process_id, 0, "Process id");
DEFINE_uint64(instance_id, 0, "Instance id (this is to properly set the RPCs");

using rpc_handle = erpc::Rpc<erpc::CTransport>;

class app_context {
public:
  app_context() = default;
  rpc_handle *rpc;
  int node_id, thread_id;
  int instance_id; /* instance id used to create the rpcs */
  ;
  /* map of <node_id, session_num> that maintains connections */
  std::unordered_map<int, int> connections;

  void create_session(const std::string &uri, int rem_node_id) {
    rpc->retry_connect_on_invalid_rpc_id = true;
    std::string middle_uri = uri + ":" + std::to_string(kUDPPort);
    fmt::print("[{}] uri={}\n", __PRETTY_FUNCTION__, middle_uri);
    int session_num = rpc->create_session(middle_uri, instance_id);
    while (!rpc->is_connected(session_num)) {
      rpc->run_event_loop_once();
    }
    fmt::print("[{}] connected to uri={} w/ remote RPC_id={} session_num={}\n",
               __func__, uri, instance_id, session_num);
    connections.insert({rem_node_id, session_num});
  }

  // delete copy/move constructors/assignments
  app_context(const app_context &) = delete;
  app_context(app_context &&) = delete;
  app_context &operator=(const app_context &) = delete;
  app_context &operator=(app_context &&) = delete;
  ~app_context() { delete rpc; }
};

enum chain_replication {
  head = 0,
  tail = 1,
  middle // FIXME: @dimitra that is only for testing
};

void head_func(std::shared_ptr<app_context> &ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  ctx->create_session(uri, 1 /* node 1 */);
  ctx->rpc->run_event_loop(100000);
}

void tail_func(std::shared_ptr<app_context> &ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  ctx->rpc->run_event_loop(100);
  ctx->create_session(uri, 0 /* node 0 */);
  ctx->rpc->run_event_loop(100000);
}
void middle_func(std::shared_ptr<app_context> &ctx, const std::string &uri1,
                 const std::string &uri2) {}

static void sm_handler(int local_session, erpc::SmEventType, erpc::SmErrType,
                       void *) {

  // fmt::print("[{}].\n", __PRETTY_FUNCTION__);
}

void proto_func(size_t thread_id, erpc::Nexus *nexus) {
  fmt::print("[{}]\tthread_id={} starts\n", __PRETTY_FUNCTION__, thread_id);
  auto ctx = std::make_shared<app_context>();
  ctx->instance_id = FLAGS_instance_id;
  ctx->rpc = new rpc_handle(nexus, static_cast<void *>(ctx.get()),
                            ctx->instance_id, sm_handler);

  /* give some time before we start */
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2000ms);
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

void req_handler(erpc::ReqHandle *req_handle, void * /*TODO: appcontext */) {
  auto &resp = req_handle->pre_resp_msgbuf;
#if 0
	rpc->resize_msg_buffer(&resp, kMsgSize);
	sprintf(reinterpret_cast<char *>(resp.buf), "hello");

	rpc->enqueue_response(req_handle, &resp);
#endif
}

int main(int args, char *argv[]) {
  signal(SIGINT, ctrl_c_handler);
  gflags::ParseCommandLineFlags(&args, &argv, true);

  erpc::rt_assert(FLAGS_resp_size <= erpc::CTransport::kMTU, "Resp too large");
  erpc::rt_assert(FLAGS_window_size <= kAppMaxWindowSize, "Window too large");

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
  std::vector<std::shared_ptr<app_context>> ctxs;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(std::thread(proto_func, i, &nexus));
    erpc::bind_to_core(threads[i], numa_node, i);
  }

  for (auto &thread : threads)
    thread.join();

  return 0;
}
