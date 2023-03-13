#pragma once
#include "rpc.h"
#include <fmt/printf.h>
#include <stdio.h>

static const std::string kdonna = "129.215.165.54";
static const std::string kmartha = "129.215.165.53";
static const std::string krose = "129.215.165.52";
static const int kdonna_id = 0;
static const int kmartha_id = 1;
static const int krose_id = 2;

static constexpr uint16_t kUDPPort = 31850;
static constexpr size_t kMsgSize = 128;

static constexpr int kReqSend = 1;
static constexpr int kReqCommit = 2;
static constexpr int PRINT_BATCH = 50000;
static constexpr int numa_node = 0;
static constexpr int tot_nodes = 3;
static constexpr int tot_acks = 4; /* 2 reqs (one from each node) and 2 acks for
                                      my req (one from each node) */

static void sm_handler(int local_session, erpc::SmEventType, erpc::SmErrType,
                       void *) {}

using rpc_handle = erpc::Rpc<erpc::CTransport>;

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
