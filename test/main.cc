#include <fmt/printf.h>
#include "rpc.h"
#include <gflags/gflags.h>
#include <signal.h>
#include <cstring>
#include "util/numautils.h"

static constexpr size_t kAppEvLoopMs = 1000;    // Duration of event loop
static constexpr bool kAppVerbose = false;      // Print debug info on datapath
static constexpr double kAppLatFac = 10.0;      // Precision factor for latency
static constexpr size_t kAppReqType = 1;        // eRPC request type
static constexpr size_t kAppMaxWindowSize = 32; // Max pending reqs per client
static constexpr int numa_node = 0;

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(window_size, 1, "Outstanding requests per client");
DEFINE_uint64(req_size, 64, "Size of request message in bytes");
DEFINE_uint64(resp_size, 32, "Size of response message in bytes");
DEFINE_uint64(process_id, 0, "Process id");

void proto_func(size_t thread_id)
{
    fmt::print("{} thread_id={}\n", __PRETTY_FUNCTION__, thread_id);
}

void ctrl_c_handler(int)
{
    fmt::print("{} ctrl-c .. exiting\n", __PRETTY_FUNCTION__);
    exit(1);
}

int main(int args, char *argv[])
{
    signal(SIGINT, ctrl_c_handler);
    gflags::ParseCommandLineFlags(&args, &argv, true);

    erpc::rt_assert(FLAGS_resp_size <= erpc::CTransport::kMTU, "Resp too large");
    erpc::rt_assert(FLAGS_window_size <= kAppMaxWindowSize, "Window too large");

    size_t num_threads = FLAGS_process_id == 0 ? FLAGS_num_server_threads : FLAGS_num_client_threads;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < num_threads; i++)
    {
        threads.emplace_back(std::thread(proto_func, i));
        erpc::bind_to_core(threads[i], numa_node, i);
    }

    for (auto& thread : threads)
        thread.join();

    return 0;
}
