#pragma once
#include "common.h"
#include "msg.h"
#include "util/numautils.h"
#include <chrono>
#include <cstring>
#include <fmt/printf.h>
#include <gflags/gflags.h>
#include <signal.h>
#include <thread>

class app_context {
public:
  struct protocol_metadata {
    int cmt_rd = -1, cur_rd = 0;
    std::vector<int> followers;

    using round = uint64_t;
    using batch = std::unique_ptr<uint8_t[]>;

    struct metadata {
      using sender_id = int;
      explicit metadata(int sender, batch req) {
        ucmt_reqs.insert({sender, std::move(req)});
        nb_cmts = 0;
        nb_acks = 0;
        under_commit = false;
      }
      // number of acks for requests
      // (from nodes that received this req + requests this node has received)
      uint64_t nb_acks = 0;
      // number of nodes who are ready to commit this round
      // any node who received all messages of this round is ready to commit
      uint64_t nb_cmts = 0;
      bool under_commit = false;

      std::unordered_map<sender_id, batch>
          ucmt_reqs; /* uncommited reqs for this round */
    };
    std::unordered_map<round, std::unique_ptr<metadata>> proto_meta;

    bool can_commit(const int nodes, int cns_rd) {
#ifdef PRINT_DEBUG
      fmt::print("[{}] cmt_rd+1={} nb_acks={}, cns_rd={}\n", __func__,
                 (cmt_rd + 1), proto_meta[cns_rd]->nb_acks, cns_rd);
#endif
#if 0
      if (cns_rd > cmt_rd + 1)
        return false;
      if (proto_meta[cmt_rd + 1]->nb_acks == nodes) {
	      if (proto_meta[cmt_rd + 1]->under_commit) {
		      fmt::print("[{}] HEEEEEEEEEEERE \n", __PRETTY_FUNCTION__);
		      return false;
	      }
	      proto_meta[cmt_rd + 1]->under_commit = true;
#endif
      if (proto_meta[cns_rd]->nb_acks == nodes) {
        if (proto_meta[cns_rd]->under_commit) {
          fmt::print("[{}] HEEEEEEEEEEERE \n", __PRETTY_FUNCTION__);
          return false;
        }
        proto_meta[cns_rd]->under_commit = true;
        return true;
      }
      return false;
      // return (proto_meta[cns_rd + 1]->nb_acks == nodes);
    }

    bool is_round_completed(const int nodes) {
      static uint64_t cnt = 0;
      if (proto_meta.find(cmt_rd + 1) == proto_meta.end()) {
        fmt::printf("[{}] entry with {} has been erased before.\n", __func__,
                    (cmt_rd + 1));
        return false;
      }
      if (proto_meta[cmt_rd + 1]->nb_cmts == nodes) {
        if ((cnt % PRINT_BATCH) == 0) {
          fmt::print("[{}] we *COMMIT* rnd={} with nb_cmts={}\n", __func__,
                     (cmt_rd + 1), proto_meta[(cmt_rd + 1)]->nb_cmts);
        }
        cnt++;
        proto_meta.erase(cmt_rd + 1);
        cmt_rd++;
        return true;
      }
      cnt++;
      //      fmt::print("[{}] nothing to commit for (cmt_rd+1)={},
      //      nb_cmts={}\n", __func__, (cmt_rd+1), proto_meta[cmt_rd +
      //      1]->nb_cmts);
      return false;
    }

    void add_recv_req(const int sender, batch reqs, const int rd) {
#ifdef PRINT_DEBUG
      fmt::print("[{}] create proto_meta for rd={}\n", __PRETTY_FUNCTION__, rd);
#endif
      if (proto_meta.find(rd) == proto_meta.end())
        proto_meta.insert(
            {rd, std::make_unique<metadata>(sender, std::move(reqs))});
      else
        proto_meta[rd]->ucmt_reqs.insert({sender, std::move(reqs)});
    }

    void commit_completed_rds(const int nodes) {
      while (cmt_rd < cur_rd) {
        if (!is_round_completed(nodes))
          return;
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

void send_cmt(int cmt_idx, const std::vector<int> &dest_ids, app_context *ctx);

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

  //  fmt::print("[{}] got ack for cns_rd={} from node={}\n", __func__, cns_rd,
  //  sender);
  ctx->metadata->proto_meta[cns_rd]->nb_cmts++;
  //   fmt::print("[{}] ack-ed cns_rd={} nb_cmts={} from node={}\n", __func__,
  //   cns_rd, ctx->metadata->proto_meta[cns_rd]->nb_cmts, sender);
  if ((count % PRINT_BATCH) == 0) {
    fmt::print("[{}] ack-ed cns_rd={} nb_cmts={} from node={}\n", __func__,
               cns_rd, ctx->metadata->proto_meta[cns_rd]->nb_cmts, sender);
  }
  ctx->metadata->commit_completed_rds(tot_acks);
  delete tag;
  count++;
}

void send_cmt(int cmt_idx, const std::vector<int> &dest_ids, app_context *ctx) {
  // construct message
  auto msg_buf = std::make_unique<msg>();
  msg_buf->hdr.src_node = ctx->node_id;
  msg_buf->hdr.dest_node = -1; /* broadcast */
  msg_buf->hdr.seq_idx = cmt_idx;
  msg_buf->hdr.msg_size = kMsgSize;
  for (auto &dest_id : dest_ids) {
#ifdef PRINT_DEBUG
    fmt::print("[{}] COMMIT to dest_id={} for cmt_idx={}\n",
               __PRETTY_FUNCTION__, dest_id, cmt_idx);
#endif
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

/* ack that the node received the req */
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

  /* prepare response */
  auto *response = reinterpret_cast<cmt_msg *>(tag->resp.buf);
  int cns_rd = response->cns_rd;

  ctx->metadata->proto_meta[cns_rd]->nb_acks++;
#if 1
  if ((count % PRINT_BATCH) == 0) {
    fmt::print("[{}] cns_rd={} from node={} nb_acks={} nb_cmts={}\n",
               __PRETTY_FUNCTION__, cns_rd, response->hdr.src_node,
               ctx->metadata->proto_meta[cns_rd]->nb_acks,
               ctx->metadata->proto_meta[cns_rd]->nb_cmts);
  }
#endif
  int sender = response->hdr.src_node;
  /* update the nb_acks */
  bool can_cmt = ctx->metadata->can_commit(tot_acks, cns_rd);
  delete tag;
  count++;
  if (can_cmt) {
    // fmt::print("[{}] cns_rd={} is eligible for commit\n",
    // __PRETTY_FUNCTION__, cns_rd);
    //	ctx->metadata->proto_meta[cns_rd]->nb_cmts++;
    /* all nodes have received all messages so sent commit-msg for this round */
    send_cmt(cns_rd, ctx->metadata->followers, ctx);
  }
}
