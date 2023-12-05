#include "common.h"
// #include "crypto/hmac_lib.h"
#include "log.h"
#include "msg.h"
#include "tnic_mock_api/api.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

static constexpr int kReqPropose = 1;
static constexpr int kReqTerminate = 5;
static constexpr int kPrintBatch = 50000;
static constexpr int numa_node =
    0; /*@dimitra: currently only 0-numa is supported */
static constexpr uint8_t kDefaultCmd[p_msg::CmdSize] = {0x0};

DEFINE_uint64(num_server_threads, 1, "Number of threads at the server machine");
DEFINE_uint64(num_client_threads, 1, "Number of threads per client machine");
DEFINE_uint64(process_id, 0, "Process id");
DEFINE_uint64(reqs_num, 5e6, "Number of reqs");

using rpc_handle = erpc::Rpc<erpc::CTransport>;

enum pb { leader = 0, follower = 1 };

// forward decls
class app_context;
bool log_audit(app_context *ctx);
void decode_print_ctx(const char *ctx_ptr);
std::tuple<uint32_t, uint32_t, uint32_t>
decode_print_ctx_leader(const char *ctx_ptr);

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
    rpc->free_msg_buffer(req);
    rpc->free_msg_buffer(resp);
  }
};

class app_context {
public:
  struct protocol_metadata {
    struct audit_protocol {
      int last_log_seq = 0;
    };

    std::map<int, struct audit_protocol> witness_meta;
    int last_state_cmt = 0;
    int leader = pb::follower;
    bool is_leader() { return leader == pb::leader; }
    uint32_t cmt_idx = 0;
    std::vector<int> followers;
    uint8_t mock_h1[p_metadata::HashSize] = {0x0};
    uint8_t mock_h2[p_metadata::HashSize] = {0x0};
    static constexpr size_t LogEntry = kMsgSize * msg_manager::batch_count;

    std::unique_ptr<trusted_log<LogEntry>> tlog;

    protocol_metadata() {
      witness_meta.emplace(std::make_pair(1 /* node id */, audit_protocol()));
      witness_meta.emplace(std::make_pair(2 /* node id */, audit_protocol()));
      tlog = std::make_unique<trusted_log<LogEntry>>(
          (LogEntry * FLAGS_reqs_num * 4) / msg_manager::batch_count);
    }
    uint8_t my_last_state[p_metadata::HashSize] = {0x0};
    uint8_t f1_last_state[p_metadata::HashSize] = {0x0};
    uint8_t f2_last_state[p_metadata::HashSize] = {0x0};
    uint8_t leader_last_state[p_metadata::HashSize] = {0x0};

    using req_id = int;
    using nb_acks = int;

    bool validate_log_entry(int cur_idx) {
      if (cur_idx == 0)
        return true;
      auto prev_idx = cur_idx - 1;
      std::unique_ptr<char[]> prev_entry =
          std::make_unique<char[]>(sizeof(trusted_log<LogEntry>::log_entry));
      tlog->serialize_entry(prev_entry.get(), prev_idx);

#ifdef FPGA
#warning "FPGA-version is enabled"
      auto [c_digest, len] = tnic_api::FPGA_get_attestation(
          reinterpret_cast<uint8_t *>(prev_entry.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#elif SGX
#warning "SGX-version is enabled"
      auto [c_digest, len] = tnic_api::SGX_get_attestation(
          reinterpret_cast<uint8_t *>(prev_entry.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#elif AMD
#warning "AMDSEV-version is enabled"
      auto [c_digest, len] = tnic_api::AMDSEV_get_attestation(
          reinterpret_cast<uint8_t *>(prev_entry.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#else
      auto [c_digest, len] = tnic_api::native_get_attestation(
          reinterpret_cast<uint8_t *>(prev_entry.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#endif
      /*
      auto [c_digest, len] =
          hmac_sha256(reinterpret_cast<uint8_t *>(prev_entry.get()),
                      sizeof(trusted_log<LogEntry>::log_entry));
        */
      char *ctx_ptr = nullptr;
      size_t ctx_sz = 0;
      tlog->print_entry_at(cur_idx, ctx_ptr, ctx_sz);
      char *cur_digest = tlog->get_entry_at(cur_idx) +
                         sizeof(trusted_log<LogEntry>::log_entry::sequencer) +
                         trusted_log<LogEntry>::log_entry::CtxSize;
      if (::memcmp(c_digest.data(), cur_digest, len) != 0) {
        fmt::print("[{}] c_digest != cur_digest\nc_digest=",
                   __PRETTY_FUNCTION__);
        for (auto i = 0ULL; i < len; i++) {
          fmt::print("{}", c_digest.data()[i]);
        }
        fmt::print("\ncur_digest=");
        for (auto i = 0ULL; i < len; i++) {
          fmt::print("{}", reinterpret_cast<unsigned char *>(cur_digest)[i]);
        }
        fmt::print("\n");
        return false;
      }
      return true;
    }

    bool log(char *context_data, size_t ctx_data_sz, char *authenticator) {
      auto *tail = tlog->get_tail();
      std::unique_ptr<char[]> tail_serialized =
          std::make_unique<char[]>(sizeof(trusted_log<LogEntry>::log_entry));
      tlog->serialize_tail(tail_serialized.get());
      // TODO: digest of tail + current entry
#ifdef FPGA
#warning "FPGA-version is enabled"
      auto [c_digest, len] = tnic_api::FPGA_get_attestation(
          reinterpret_cast<uint8_t *>(tail_serialized.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#elif SGX
#warning "SGX-version is enabled"
      auto [c_digest, len] = tnic_api::SGX_get_attestation(
          reinterpret_cast<uint8_t *>(tail_serialized.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#elif AMD
#warning "AMDSEV-version is enabled"
      auto [c_digest, len] = tnic_api::AMDSEV_get_attestation(
          reinterpret_cast<uint8_t *>(tail_serialized.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#else
      auto [c_digest, len] = tnic_api::native_get_attestation(
          reinterpret_cast<uint8_t *>(tail_serialized.get()),
          sizeof(trusted_log<LogEntry>::log_entry));
#endif
      /*
      auto [c_digest, len] =
          hmac_sha256(reinterpret_cast<uint8_t *>(tail_serialized.get()),
                      sizeof(trusted_log<LogEntry>::log_entry));
        */
      trusted_log<LogEntry>::log_entry e;
      e.sequencer = tlog->get_log_size();
      // c_digest is a vector<char>
      ::memcpy(e.c_digest, c_digest.data(), len);
#if 0
      fmt::print("[{}] digest to be added:", __func__);
      for (auto i = 0ULL; i < len; i++) {
        fmt::print("{}", c_digest.data()[i]);
      }
      fmt::print("\n");
      fmt::print("[{}] added entry digest:", __func__);
      for (auto i = 0ULL; i < len; i++) {
        fmt::print("{}", reinterpret_cast<unsigned char *>(e.c_digest)[i]);
      }
      fmt::print("\n");
#endif
      ::memcpy(e.authenticator, authenticator,
               trusted_log<LogEntry>::log_entry::AuthSize);
      if (ctx_data_sz > trusted_log<LogEntry>::log_entry::CtxSize) {
        fmt::print("[{}] error ctx_data_sz {} > CtxSize {}\n",
                   __PRETTY_FUNCTION__, ctx_data_sz,
                   trusted_log<LogEntry>::log_entry::CtxSize);
        exit(128);
      }
      ::memcpy(e.context, context_data, ctx_data_sz);
      if (!tlog->append(e)) {
        fmt::print("[{}] log is full\n", __PRETTY_FUNCTION__);
        return false;
      }
      return true;
    }
    bool update_f_hashes(int idx, int src_node, uint8_t *hash_ptr) {
      if (src_node == 1) {
        ::memcpy(mock_h1, hash_ptr, ack_msg::HashSize);
        return true;
      }
      if (src_node == 2) {
        ::memcpy(mock_h2, hash_ptr, ack_msg::HashSize);
      }
      return true;
    }
  };

  uint8_t *get_f1_hash() { return metadata->mock_h1; }

  uint8_t *get_f2_hash() { return metadata->mock_h2; }

  rpc_handle *rpc = nullptr;
  int node_id = 0, thread_id = 0;
  int instance_id = 0; /* instance id used to create the rpcs */
  msg_manager *batcher = nullptr;
  protocol_metadata *metadata = nullptr;
  uint32_t count_replies = 0;
  uint32_t finished_nodes = 0;
  bool stop = false;

  uint64_t enqueued_msgs_cnt = 0;

  /* map of <node_id, session_num> that maintains connections */
  std::unordered_map<int, int> connections;

  ~app_context() {
    fmt::print("{} ..\n", __func__);
    delete rpc;
  }
};

static void cont_func_terminate(void *context, void *t) {
  auto *ctx = static_cast<app_context *>(context);
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr (going to sleep for 10s)\n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  auto *tag = static_cast<rpc_buffs *>(t);
  ctx->finished_nodes++;
}

static void cont_func_prp(void *context, void *t) {
  static uint64_t count = 0;
  auto *ctx = static_cast<app_context *>(context);
  if (ctx == nullptr) {
    fmt::print("{} ctx==nullptr (going to sleep for 10s)\n", __func__);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10000ms);
  }
  auto *tag = static_cast<rpc_buffs *>(t);

  auto *response = tag->resp.buf;
  auto buf_sz = tag->resp.get_data_size();
  // validate (TNIC)
#ifdef FPGA
#warning "FPGA-version is enabled"
  auto [calc_mac, len] =
      tnic_api::FPGA_get_attestation(response, (buf_sz - _hmac_size));
#elif SGX
#warning "SGX-version is enabled"
  auto [calc_mac, len] =
      tnic_api::SGX_get_attestation(response, (buf_sz - _hmac_size));
#elif AMD
#warning "AMDSEV-version is enabled"
  auto [calc_mac, len] =
      tnic_api::AMDSEV_get_attestation(response, (buf_sz - _hmac_size));
#else
  auto [calc_mac, len] =
      tnic_api::native_get_attestation(response, (buf_sz - _hmac_size));
#endif
  // auto [calc_mac, len] = hmac_sha256(response, (buf_sz - _hmac_size));
  if (::memcmp(response + (buf_sz - _hmac_size), calc_mac.data(), len) != 0) {
    fmt::print("[{}] error on HMAC validation\n", __PRETTY_FUNCTION__);
  }

  // log the action
  auto ctx_data = response;
  auto ctx_data_sz = buf_sz - _hmac_size;
  auto authenticator = response + (buf_sz - _hmac_size);
  ctx->metadata->log(reinterpret_cast<char *>(ctx_data), ctx_data_sz,
                     reinterpret_cast<char *>(authenticator));

  ctx->count_replies++;
  if ((count % kPrintBatch) == 0) {
    fmt::print("[{}] ack-ed {} prp_reqs\n", __func__, count);
  }
#if 0
  int cmt = f_get_cmt(response);
  int sender_id = f_get_sender(response);
  bool ack = f_get_ack_val(response);
  if (ack == false) {
    fmt::print("[{}] wrong!\n", __PRETTY_FUNCTION__);
  }

  //TODO: not sure if we need to have the 'signed' state here
  // @dimitra: state is a pointer to the memory area, if 'ack' goes out of scope
  // we will seg-fault here
  uint8_t *state = f_get_state(response);

  // TODO: store prev followers hashes and validate and apply req
  ctx->metadata->update_f_hashes(cmt, sender_id, state);
#endif
  delete tag;
  count++;
}

void send_req_finito(int idx, const std::vector<int> &dest_ids,
                     app_context *ctx) {
  fmt::print("[{}] \n", __func__);
  for (auto &dest_id : dest_ids) {
    // construct message
    rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
    buffs->alloc_req_buf(kMsgSize, ctx->rpc);
    buffs->alloc_resp_buf(kMsgSize + _hmac_size, ctx->rpc);

    ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqTerminate,
                              &buffs->req, &buffs->resp, cont_func_terminate,
                              reinterpret_cast<void *>(buffs));
  }
}

void send_req(int idx, const std::vector<int> &dest_ids, app_context *ctx) {
  // construct message
  auto msg_buf = std::make_unique<p_msg>();
  auto char_msg_buf = std::make_unique<uint8_t[]>(sizeof(p_msg));

  // fill in the message buf with data
  ::memcpy(msg_buf->cmd, kDefaultCmd, p_msg::CmdSize);
  ::memcpy(msg_buf->output, &idx, sizeof(uint32_t));
  ::memcpy(&(msg_buf->meta.cnt), &idx, sizeof(uint32_t));
  ::memcpy(msg_buf->meta.f1_prev, ctx->get_f1_hash(), p_metadata::HashSize);
  ::memcpy(msg_buf->meta.f2_prev, ctx->get_f2_hash(), p_metadata::HashSize);

  memcpy_p_msg_into_buffer(*(msg_buf.get()), char_msg_buf.get());
#if 0
  std::cout << "MY DEBUG:\n";
  decode_print_ctx(reinterpret_cast<char *>(char_msg_buf.get()));
  std::cout << "MY DEBUG ends\n";
#endif

  if (p_get_msg_buf_sz() != sizeof(p_msg)) {
    fmt::print(
        "[{}] buf sizes missmatch (check alignment and padding!) {} vs {}\n",
        __PRETTY_FUNCTION__, p_get_msg_buf_sz(), sizeof(p_msg));
    exit(128);
  }
  if (!ctx->batcher->enqueue_req(
          reinterpret_cast<uint8_t *>(char_msg_buf.get()), sizeof(p_msg))) {
    for (auto &dest_id : dest_ids) {
      // needs to send
      rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
      buffs->alloc_req_buf(kMsgSize * msg_manager::batch_count + _hmac_size,
                           ctx->rpc);
      buffs->alloc_resp_buf(kAckSize + _hmac_size, ctx->rpc);

      ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
               kMsgSize * msg_manager::batch_count);

      // sign before transmission
#ifdef FPGA
#warning "FPGA-version is enabled"
      auto [mac, len] = tnic_api::FPGA_get_attestation(
          buffs->req.buf, kMsgSize * msg_manager::batch_count);
#elif SGX
#warning "SGX-version is enabled"
      auto [mac, len] = tnic_api::SGX_get_attestation(
          buffs->req.buf, kMsgSize * msg_manager::batch_count);
#elif AMD
#warning "AMDSEV-version is enabled"
      auto [mac, len] = tnic_api::AMDSEV_get_attestation(
          buffs->req.buf, kMsgSize * msg_manager::batch_count);
#else
      auto [mac, len] = tnic_api::native_get_attestation(
          buffs->req.buf, kMsgSize * msg_manager::batch_count);
#endif
      /*
      auto [mac, len] =
          hmac_sha256(buffs->req.buf, kMsgSize * msg_manager::batch_count);
          */
      if (len != _hmac_size) {
        fmt::print("[{}] len ({}) != _hmac_size ({})\n", __PRETTY_FUNCTION__,
                   len, _hmac_size);
      }
      ::memcpy(buffs->req.buf + kMsgSize * msg_manager::batch_count, mac.data(),
               len);
      // enqueue_req
      ctx->rpc->enqueue_request(ctx->connections[dest_id], kReqPropose,
                                &buffs->req, &buffs->resp, cont_func_prp,
                                reinterpret_cast<void *>(buffs));
    }
    ctx->batcher->empty_buff();
    ctx->batcher->enqueue_req(reinterpret_cast<uint8_t *>(msg_buf.get()),
                              p_get_msg_buf_sz());
    ctx->enqueued_msgs_cnt++;
  }
  log_audit(ctx);
}

void flash_batcher(const std::vector<int> &dest_ids, app_context *ctx) {
  // needs to send
  for (auto &dest_id : dest_ids) {
    rpc_buffs *buffs = new rpc_buffs(ctx->rpc);
    buffs->alloc_req_buf(kMsgSize * ctx->batcher->cur_idx + _hmac_size,
                         ctx->rpc);
    buffs->alloc_resp_buf(kAckSize + _hmac_size, ctx->rpc);

    ::memcpy(buffs->req.buf, ctx->batcher->serialize_batch(),
             kMsgSize * ctx->batcher->cur_idx);

#ifdef FPGA
#warning "FPGA-version is enabled"
    auto [mac, len] = tnic_api::FPGA_get_attestation(
        buffs->req.buf, kMsgSize * ctx->batcher->cur_idx);
#elif SGX
#warning "SGX-version is enabled"
    auto [mac, len] = tnic_api::SGX_get_attestation(
        buffs->req.buf, kMsgSize * msg_manager::batch_count);
#elif AMD
#warning "AMDSEV-version is enabled"
    auto [mac, len] = tnic_api::AMDSEV_get_attestation(
        buffs->req.buf, kMsgSize * msg_manager::batch_count);
#else
    auto [mac, len] = tnic_api::native_get_attestation(
        buffs->req.buf, kMsgSize * ctx->batcher->cur_idx);
#endif
    /*
  auto [mac, len] =
      hmac_sha256(buffs->req.buf, kMsgSize * ctx->batcher->cur_idx);
      */
    if (len != _hmac_size) {
      fmt::print("[{}] len ({}) != _hmac_size ({})\n", __PRETTY_FUNCTION__, len,
                 _hmac_size);
    }
    ::memcpy(buffs->req.buf + kMsgSize * ctx->batcher->cur_idx, mac.data(),
             len);
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
  fmt::print("{} execute {} reqs\n", __PRETTY_FUNCTION__, FLAGS_reqs_num);
  ctx->rpc->run_event_loop(100);
  auto start = std::chrono::steady_clock::now();
  int iterations = FLAGS_reqs_num;

  for (auto i = 0ULL; i < FLAGS_reqs_num; i++) {
    send_req(i, followers, ctx);
    poll(ctx);
  }
  flash_batcher(followers, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  auto &log_handle = ctx->metadata->tlog;
  for (;;) {
    ctx->rpc->run_event_loop_once();
    log_audit(ctx);
    if (ctx->count_replies * msg_manager::batch_count == iterations * 2) {
      send_req_finito(FLAGS_reqs_num + 1, followers, ctx);
      ctx->rpc->run_event_loop_once();
      break;
    }
  }
  for (;;) {
    log_audit(ctx);
    ctx->rpc->run_event_loop_once();
    if (log_handle->get_log_size() == iterations * 2)
      if (ctx->finished_nodes == 2)
        break;
  }

  fmt::print("[{}] ctx->count_replies={}\n", __PRETTY_FUNCTION__,
             ctx->count_replies);
  auto end = std::chrono::steady_clock::now();
  auto duration2 =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  const std::chrono::duration<double> elapsed_seconds{end - start};
  fmt::print("[{}] elapsed={}/{} for {} iterations (avg_lat={} sec\t {}us)\n",
             __PRETTY_FUNCTION__, elapsed_seconds.count(), duration2.count(),
             iterations, (elapsed_seconds.count() * 1.0) / (iterations * 1.0),
             ((duration2.count() * 1.0) / (iterations * 1.0)));
}

void follower_func(app_context *ctx, const std::string &uri) {
  fmt::print("[{}]\tinstance_id={}\tthread_id={}\turi={}.\n",
             __PRETTY_FUNCTION__, ctx->instance_id, ctx->thread_id, uri);
  // create_session(uri, pb::leader /* node 0 */, ctx);
  ctx->rpc->run_event_loop(100);
  fmt::print("{} execute {} reqs\n", __PRETTY_FUNCTION__, FLAGS_reqs_num);
  for (auto i = 0ULL; i < FLAGS_reqs_num; i++) {
    // send_req(i, 0, ctx);
    poll(ctx);
  }
  // flash_batcher(0, ctx);
  fmt::print("{} polls until the end ...\n", __func__);
  for (;;) {
    ctx->rpc->run_event_loop_once();
    if (ctx->stop)
      break;
  }
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

  /* we can start */

  if (FLAGS_process_id == pb::leader) {
    using uri_tuple = std::tuple<int, std::string>;
    std::vector<std::tuple<int, std::string>> uris{
        uri_tuple{1, std::string{kmartha}}, uri_tuple{2, std::string{kamy}}};

    leader_func(ctx, uris);
  } else {
    follower_func(ctx, std::string{kdonna});
  }

  fmt::print("[{}]\tthread_id={} exits.\n", __PRETTY_FUNCTION__, thread_id);
  delete ctx;
}

void ctrl_c_handler(int) {
  fmt::print("{} ctrl-c .. exiting\n", __PRETTY_FUNCTION__);
  exit(1);
}

bool apply_protocol(std::unique_ptr<p_msg> recv_msg, app_context *ctx,
                    bool last_req) {
  // apply the protocol (deterministic), increase the counter
  ctx->metadata->cmt_idx++;
  return true;
}
#include <tuple>
std::tuple<uint32_t, uint32_t, uint32_t>
decode_print_ctx_leader(const char *ctx_ptr) {
  // fmt::print("[{}]\n", __func__);
  auto cmd_ptr = ctx_ptr;
  bool res = *(reinterpret_cast<const bool *>(cmd_ptr));
#if 0
  fmt::print("ack: {}\n", res);
  fmt::print("\n");
#endif
  if (!res) {
    fmt::print("[{}] ack equals false\n", __PRETTY_FUNCTION__);
    exit(128);
  }

  //  fmt::print("output:");
  auto output_ptr = cmd_ptr + ack_msg::AckSize;
  uint32_t output_val = *(reinterpret_cast<const uint32_t *>(output_ptr));
  // fmt::print("{}", output_val);
  // fmt::print("\n");

  // fmt::print("cmt:");
  auto *cmt_ptr = cmd_ptr + ack_msg::AckSize + ack_msg::OutSize;
  uint32_t cmt_val;
  ::memcpy(&cmt_val, cmt_ptr, sizeof(uint32_t));
  // fmt::print("{}", cmt_val);
  // fmt::print("\n");

  // fmt::print("sender:");
  auto *sender_ptr =
      cmd_ptr + ack_msg::AckSize + ack_msg::OutSize + sizeof(ack_msg::cmt);
  uint32_t sender_val = *(reinterpret_cast<const uint32_t *>(sender_ptr));
  // fmt::print("{}", sender_val);
  // fmt::print("\n");

  // fmt::print("leader cmd:");
  auto *org_cmd = sender_ptr + ack_msg::HashSize;
  if (::memcmp(org_cmd, kDefaultCmd, p_msg::CmdSize) != 0) {
    fmt::print("[{}] Wrong cmd\n", __PRETTY_FUNCTION__);
    exit(128);
  }
  // fmt::print(" default");
#if 0
	for (auto i = 0; i < p_msg::CmdSize; i++) {
		fmt::print("{}", reinterpret_cast<const char*>(org_cmd)[i]);
	}
#endif
  // fmt::print("\n");
  return {cmt_val, output_val, sender_val};
}

void decode_print_ctx(const char *ctx_ptr) {
  // fmt::print("[{}]\n", __func__);
  auto cmd_ptr = ctx_ptr;
  /*
  fmt::print("cmd:");
  for (auto i = 0; i < p_msg::CmdSize; i++) {
    fmt::print("{}", cmd_ptr[i]);
  }
  fmt::print("\n");

  fmt::print("output:");
  */
  auto output_ptr = cmd_ptr + p_msg::CmdSize;
  // fmt::print("{}", *(reinterpret_cast<const uint32_t *>(output_ptr)));
  // fmt::print("\n");

  // fmt::print("msg_cnt:");
  auto *msg_cnt_ptr = cmd_ptr + p_msg::CmdSize + p_msg::OutSize;
  // fmt::print("{}", *(reinterpret_cast<const uint32_t *>(msg_cnt_ptr)));
  // fmt::print("\n");
}

bool log_audit(app_context *ctx) {
#if LOG_AUDIT
#warning "log-audit is ON"
  /* Keep sequence number of log n plus the 'expected' state after n
   * Send the n to the participant and read from [n, n'] where n' the last entry
   * Then apply the states using the reference implementation to audit the
   * protocol execution
   */
  auto &log_handle = ctx->metadata->tlog;
  auto tail_idx = log_handle->get_log_size();
  if (ctx->metadata->last_state_cmt == tail_idx)
    return true;
  /*
   fmt::print("[{}] ############ START ############ tail_idx={}\n", __func__,
              tail_idx);
 */
  for (auto i = ctx->metadata->last_state_cmt; i < tail_idx; i++) {
    char *ctx_ptr = nullptr;
    size_t ctx_sz = 0;

    // check the HMAC
    ctx->metadata->validate_log_entry(i);
    log_handle->print_entry_at(i, ctx_ptr, ctx_sz);
    // decode log entry
    auto res_cmt_output_sender = decode_print_ctx_leader(ctx_ptr);
    auto cmt_val = std::get<0>(res_cmt_output_sender);
    auto output_val = std::get<1>(res_cmt_output_sender);
    auto sender_val = std::get<2>(res_cmt_output_sender);

    ctx->metadata->witness_meta[sender_val].last_log_seq +=
        msg_manager::batch_count;
    // check if it is expected
    if ((cmt_val != output_val) ||
        (cmt_val != ctx->metadata->witness_meta[sender_val].last_log_seq)) {
      fmt::print("[{}] failed@i={} "
                 "cmt_val:{}\toutput_val:{}\tctx->metadata->witness_meta[{}]."
                 "last_log_seq:{}\n",
                 __PRETTY_FUNCTION__, i, cmt_val, output_val, sender_val,
                 ctx->metadata->witness_meta[sender_val].last_log_seq);
      exit(128);
    }
    // apply the cmd to the expected state
    ctx->metadata->last_state_cmt++;
  }
  /*
  fmt::print("[{}] ############ END ############ tail_idx={}\n", __func__,
             tail_idx);
  */
#endif
  return true;
}

void req_handler_finito(erpc::ReqHandle *req_handle,
                        void *context /* app_context */) {
  app_context *ctx = reinterpret_cast<app_context *>(context);
  ctx->stop = true;

  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);

  size_t buf_sz = req_handle->get_req_msgbuf()->get_data_size();

  /* enqueue ack-response */
  ack_msg ack;
  ack.sender = ctx->node_id;
  ack.cmt = ctx->metadata->cmt_idx;
  ::memcpy(&ack.output, &(ack.cmt), sizeof(uint32_t));

  // also include the original req
  ::memcpy(&ack.cmd, kDefaultCmd, p_msg::CmdSize);

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

  // sign message before transmission
  ctx->rpc->resize_msg_buffer(&resp, f_get_msg_buf_sz() + _hmac_size);
  memcpy_ack_into_buffer(ack, resp.buf);
  ctx->rpc->enqueue_response(req_handle, &resp);
}

void req_handler_prp(erpc::ReqHandle *req_handle,
                     void *context /* app_context */) {
  static uint64_t count = 0;
  app_context *ctx = reinterpret_cast<app_context *>(context);

  uint8_t *recv_data =
      reinterpret_cast<uint8_t *>(req_handle->get_req_msgbuf()->buf);

  size_t buf_sz = req_handle->get_req_msgbuf()->get_data_size();

  auto batch_sz = (buf_sz - _hmac_size) / kMsgSize;
  if ((buf_sz - _hmac_size) % kMsgSize != 0) {
    fmt::print(
        "[{}] something seems wrong (count={}) with buf_sz {} (kMsgSize {})\n",
        __PRETTY_FUNCTION__, count, buf_sz, kMsgSize);
  }

  // validate before execution (TNIC)
#ifdef FPGA
#warning "FPGA-version is enabled"
  auto [calc_mac, len] =
      tnic_api::FPGA_get_attestation(recv_data, (buf_sz - _hmac_size));
#elif SGX
#warning "SGX-version is enabled"
  auto [calc_mac, len] =
      tnic_api::SGX_get_attestation(recv_data, (buf_sz - _hmac_size));
#elif AMD
#warning "AMDSEV-version is enabled"
  auto [calc_mac, len] =
      tnic_api::AMDSEV_get_attestation(recv_data, (buf_sz - _hmac_size));
#else
  auto [calc_mac, len] =
      tnic_api::native_get_attestation(recv_data, (buf_sz - _hmac_size));
#endif
  // auto [calc_mac, len] = hmac_sha256(recv_data, (buf_sz - _hmac_size));
  if (::memcmp(recv_data + (buf_sz - _hmac_size), calc_mac.data(), len) != 0) {
    fmt::print("[{}] error on HMAC validation\n", __PRETTY_FUNCTION__);
  }

  // log the action
  char *ctx_data = reinterpret_cast<char *>(recv_data);
  decode_print_ctx(ctx_data);
  auto ctx_data_sz = buf_sz - _hmac_size;
  auto authenticator = recv_data + (buf_sz - _hmac_size);
  ctx->metadata->log(reinterpret_cast<char *>(ctx_data), ctx_data_sz,
                     reinterpret_cast<char *>(authenticator));
  // TODO: validate periodically

  // decode the (batched) msg and validate actions
  bool ok = true;
  for (auto i = 0; i < batch_sz; i++) {
    auto recv_msg = memcpy_into_p_msg(recv_data);
    ok = apply_protocol(std::move(recv_msg), ctx, (i == (batch_sz - 1)));
    if (!ok) {
      fmt::print("[{}] apply the protocol failed\n", __PRETTY_FUNCTION__);
    }
    recv_data += kMsgSize;
  }

  if (count == FLAGS_reqs_num / msg_manager::batch_count - 1)
    fmt::print("{} final count={}\n", __func__, count);
  count++;

  /* enqueue ack-response */
  ack_msg ack;
  ack.sender = ctx->node_id;
  ack.cmt = ctx->metadata->cmt_idx;
  ::memcpy(&ack.output, &(ack.cmt), sizeof(uint32_t));
  ::memcpy(&ack.ack, &(ok), sizeof(bool));

  // also include the original req
  ::memcpy(&ack.cmd, kDefaultCmd, p_msg::CmdSize);

  // HMAC-state here (TNIC but local, no sending)
#ifdef FPGA
#warning "FPGA-version is enabled"
  auto [state_mac, sz] =
      tnic_api::FPGA_get_attestation(ack.state, ack_msg::HashSize);
#elif SGX
#warning "SGX-version is enabled"
  auto [state_mac, sz] =
      tnic_api::SGX_get_attestation(ack.state, ack_msg::HashSize);
#elif AMD
#warning "AMDSEV-version is enabled"
  auto [state_mac, sz] =
      tnic_api::AMDSEV_get_attestation(ack.state, ack_msg::HashSize);
#else
  auto [state_mac, sz] =
      tnic_api::native_get_attestation(ack.state, ack_msg::HashSize);
#endif
  //  auto [state_mac, sz] = hmac_sha256(ack.state, ack_msg::HashSize);
  ::memcpy(ack.state, state_mac.data(), ack_msg::HashSize);
  if (ack.sender == 1) {
    ::memcpy(ctx->metadata->f1_last_state, state_mac.data(), ack_msg::HashSize);
  } else {
    ::memcpy(ctx->metadata->f2_last_state, state_mac.data(), ack_msg::HashSize);
  }

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

  // sign message before transmission
  ctx->rpc->resize_msg_buffer(&resp, f_get_msg_buf_sz() + _hmac_size);
  memcpy_ack_into_buffer(ack, resp.buf);
#ifdef FPGA
#warning "FPGA-version is enabled"
  auto [mac, hlen] =
      tnic_api::FPGA_get_attestation(resp.buf, f_get_msg_buf_sz());
#elif SGX
#warning "SGX-version is enabled"
  auto [mac, hlen] =
      tnic_api::SGX_get_attestation(resp.buf, f_get_msg_buf_sz());
#elif AMD
#warning "AMDSEV-version is enabled"
  auto [mac, hlen] =
      tnic_api::AMDSEV_get_attestation(resp.buf, f_get_msg_buf_sz());
#else
  auto [mac, hlen] =
      tnic_api::native_get_attestation(resp.buf, f_get_msg_buf_sz());
#endif
  // auto [mac, hlen] = hmac_sha256(resp.buf, f_get_msg_buf_sz());
  if (hlen != _hmac_size) {
    fmt::print("[{}] len ({}) != _hmac_size ({})\n", __PRETTY_FUNCTION__, hlen,
               _hmac_size);
  }
  ::memcpy(resp.buf + f_get_msg_buf_sz(), mac.data(), hlen);
  ctx->rpc->enqueue_response(req_handle, &resp);
}

int main(int args, char *argv[]) {
  signal(SIGINT, ctrl_c_handler);
  gflags::ParseCommandLineFlags(&args, &argv, true);

  size_t num_threads = FLAGS_process_id == 0 ? FLAGS_num_server_threads
                                             : FLAGS_num_client_threads;
  if (num_threads > 1) {
    fmt::print("[{}] you are testing a multi-threaded implementation\n",
               __func__);
  }

  std::string server_uri;
  if (FLAGS_process_id == 0) {
    server_uri = kdonna + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == 1) {
    server_uri = kmartha + ":" + std::to_string(kUDPPort);
  } else if (FLAGS_process_id == 2) {
    server_uri = kamy + ":" + std::to_string(kUDPPort);
  } else {
    fmt::print("[{}] not valid process_id={}\n", __func__, FLAGS_process_id);
    exit(128);
  }

  erpc::Nexus nexus(server_uri, 0, 0);
  nexus.register_req_func(kReqPropose, req_handler_prp);
  nexus.register_req_func(kReqTerminate, req_handler_finito);

  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; i++) {
    threads.emplace_back(std::thread(proto_func, i, &nexus));
    erpc::bind_to_core(threads[i], numa_node, i);
  }

  for (auto &thread : threads)
    thread.join();

  return 0;
}
