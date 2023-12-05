#pragma once
#include "msg.h"
enum chain_replication { head = 0, middle = 1, tail = 2 };
/*
 * 1) Verify payload/sender
 * 2) Verify previous execution
 * 3) Apply execution
 * 4) Sign and forward the previous payload + current output.
 */
/* msg_format:
 * 	1. original (client) request 		(PUT --- key --- val)
 * 	2. leader's action/output	        (PUT --- key --- val --- cmt)
 * 	3. <middle1 output> <middle2 output> <middle3 output>
 */

std::tuple<size_t, std::unique_ptr<uint8_t[]>>
construct_msg(int node_id, msg *m /* you can put other fields too*/) {
  size_t msg_sz =
      sizeof(msg) + sizeof(msg) + sizeof(uint32_t) + node_id * sizeof(msg);
  std::unique_ptr<uint8_t[]> ptr = std::make_unique<uint8_t[]>(msg_sz);
  //  fmt::print("[{}] idx={}\n", __func__, m->hdr.seq_idx);
  ::memcpy(ptr.get() + 2 * sizeof(msg), &(m->hdr.seq_idx), sizeof(uint32_t));
  size_t offset = 2 * sizeof(msg) + sizeof(uint32_t);
  for (auto i = 0ULL; i < node_id; i++) {
    ::memcpy(ptr.get() + offset, m, sizeof(msg));
    offset += sizeof(msg);
  }

  return std::make_tuple(msg_sz, std::move(ptr));
}

bool check_leader(uint8_t *ptr, state *st) {
  static std::atomic<int> count = 0;
  // checks that the request matches the action and the output
  std::unique_ptr<uint8_t[]> creq = std::make_unique<uint8_t[]>(sizeof(msg));
  std::unique_ptr<uint8_t[]> output =
      std::make_unique<uint8_t[]>(sizeof(msg) + sizeof(uint32_t));
  ::memcpy(creq.get(), ptr, sizeof(msg));
  ::memcpy(output.get(), ptr + sizeof(msg), sizeof(msg));
  if (::memcmp(creq.get(), output.get(), sizeof(msg)) != 0) {
    fmt::print(
        "[{}] ERROR: leader's action does not match the leader's output.\n",
        __func__);
    count++;
    return false;
  }

  st->cmt_idx++;
  uint32_t cmt_idx = 0;
  ::memcpy(&cmt_idx, (ptr + sizeof(msg) + sizeof(msg)), sizeof(uint32_t));

#ifdef DEBUG_PRINT
  fmt::print("[{} #{}] CORRECT LEADER: cmt_idx={} (from leader) (current={})\n",
             __func__, count, cmt_idx, st->cmt_idx);
#endif
  if (st->cmt_idx != cmt_idx) {
    fmt::print(
        "[{}] ERROR: leader's action {} does not match the expected leader's "
        "action {}.\n",
        __func__, cmt_idx, st->cmt_idx);
    return false;
  }
  count++;
  return true;
}

bool check_outputs(uint8_t *ptr, int node_id) {
  std::unique_ptr<uint8_t[]> creq = std::make_unique<uint8_t[]>(sizeof(msg));
  std::unique_ptr<uint8_t[]> output = std::make_unique<uint8_t[]>(sizeof(msg));
  // size_t offset = sizeof(uint32_t) + sizeof(msg);
  size_t offset = sizeof(msg);
  ::memcpy(creq.get(), ptr + offset, sizeof(msg));
  // ::memcpy(creq.get(), ptr + offset, sizeof(msg));
#if 0
  fmt::print("[{}] creq=", __func__);
  for (auto i = 0ULL; i < sizeof(msg); i++) {
    fmt::print("{}", creq.get()[i]);
  }
  fmt::print("\n .... \n");
#endif
  // offset += sizeof(msg) ;
  offset += sizeof(msg) + sizeof(uint32_t);
  for (auto i = 1ULL; i < node_id; i++) {
    ::memcpy(output.get(), ptr + offset, sizeof(msg));
    if (::memcmp(creq.get(), output.get(), sizeof(msg)) != 0) {
      fmt::print("[{}] ERROR: wtf?\n", __func__);
      fmt::print("creq=", __func__);
      for (auto i = 0ULL; i < sizeof(msg); i++) {
        fmt::print("{}", creq.get()[i]);
      }
      fmt::print("\n .... \n");
      for (auto i = 0ULL; i < sizeof(msg); i++) {
        fmt::print("{}", output.get()[i]);
      }
      fmt::print("\n .... \n");
      return false;
    }
#if 0
    for (auto i = 0ULL; i < sizeof(msg); i++) {
      fmt::print("{}", output.get()[i]);
    }
    fmt::print("\n .... \n");
#endif
    offset += sizeof(msg);
  }
  return true;
}

/* data is the verified message */
bool verify_execution(char *data, int node_id, state *st) {
#ifdef DEBUG_PRINT
  fmt::print("[{}] #1\n", __func__);
#endif
  return check_leader(reinterpret_cast<uint8_t *>(data), st);
}

std::tuple<size_t, std::unique_ptr<uint8_t[]>>
construct_msg_middle(std::unique_ptr<uint8_t[]> leader_msg, size_t msg_sz,
                     uint8_t *payload) {
  size_t alloc_sz = (HMAC_N::signature_size * sizeof(uint8_t) +
                     sizeof(uint64_t) + sizeof(msg) + sizeof(uint32_t)) /
                    sizeof(uint8_t);
#if 0
  for (auto i = 0; i < msg_sz; i++) {
    fmt::print("{}", leader_msg.get()[i]);
  }
  fmt::print("\n");
#endif
  size_t sz = (msg_sz) + alloc_sz;
  //  fmt::print("[{}] sz={}\talloc_sz={}\n", __func__, sz, alloc_sz);
  std::unique_ptr<uint8_t[]> signed_data = std::make_unique<uint8_t[]>(sz);
  std::unique_ptr<uint8_t[]> data =
      std::make_unique<uint8_t[]>(msg_sz + sizeof(msg) + sizeof(uint32_t));
  ::memcpy(data.get(), leader_msg.get(), msg_sz);
  // @dimitra: here we just copy the middle's output (copy of the leader for
  // simplicity)
  ::memcpy(data.get() + msg_sz, payload, sizeof(msg) + sizeof(uint32_t));
#if 0
  fmt::print("[{}] we copy the output=", __func__);
  for (auto i = 0ULL; i < sizeof(msg); i++) {
    fmt::print("{}", (payload)[i]);
  }
  fmt::print("\n ....>>>>...\n");
#endif
  char *privateKey = nullptr;
  bool ret = HMAC_N::sign_msg(
      data.get(), msg_sz + sizeof(msg) + sizeof(uint32_t),
      reinterpret_cast<uint8_t *>(privateKey), signed_data.get());
  if (!ret) {
    fmt::print("[{}] ERROR: sign w/ priv key failed.\n", __PRETTY_FUNCTION__);
    exit(0);
  }
#ifdef DEBUG_PRINT
  fmt::print("[{}] signing done ..\n", __func__);
#endif
  return std::make_tuple(sz, std::move(signed_data));
}

/* data is the verified message */
bool verify_execution_tail(char *data, int node_id, state *st) {
#ifdef DEBUG_PRINT
  fmt::print("[{}] #1\n", __func__);
#endif
  if (node_id == chain_replication::tail) {
    std::tuple<bool, std::unique_ptr<uint8_t[]>> result =
        msg_manager::verify(reinterpret_cast<uint8_t *>(data));

#ifdef DEBUG_PRINT
    fmt::print("[{}] #2\n", __func__);
#endif
    if (!std::get<0>(result)) {
      fmt::print("[{}] ERROR: verification failed.\n", __func__);
      return false;
    }
    if (!check_leader(std::get<1>(result).get(), st)) {
      fmt::print("[{}] ERROR: checking leader failed.\n", __func__);
      return false;
    }
#if 1
    if (!check_outputs(std::get<1>(result).get(), node_id)) {
      fmt::print("[{}] ERROR: checking outputs failed.\n", __func__);
      return false;
    }
    // fmt::print("{} OUTPUTS ALSO CORRECT ???????\n", __func__);
#endif
  } else if (node_id == chain_replication::middle) {
    return check_leader(reinterpret_cast<uint8_t *>(data), st);
  }
  return true;
}
