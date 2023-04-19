#pragma once
#include "msg.h"
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
  ::memcpy(ptr.get() + 2 * sizeof(msg), &(m->hdr.seq_idx), sizeof(uint32_t));
  size_t offset = 2 * sizeof(msg) + sizeof(uint32_t);
  for (auto i = 0ULL; i < node_id; i++) {
    ::memcpy(ptr.get() + offset, m, sizeof(msg));
    offset += sizeof(msg);
  }

  return std::make_tuple(msg_sz, std::move(ptr));
}

bool check_leader(uint8_t *ptr, state *st) {
  // checks that the request matches the action and the output
  std::unique_ptr<uint8_t[]> creq = std::make_unique<uint8_t[]>(sizeof(msg));
  std::unique_ptr<uint8_t[]> output =
      std::make_unique<uint8_t[]>(sizeof(msg) + sizeof(uint32_t));
  ::memcpy(creq.get(), ptr, sizeof(msg));
  ::memcpy(output.get(), ptr + sizeof(msg), sizeof(msg));
  if (::memcpy(creq.get(), output.get(), sizeof(msg)) != 0)
    fmt::print("{} ..\n", __func__);

  st->cmt_idx++;
  uint32_t cmt_idx = 0;
  ::memcpy(&cmt_idx, (ptr + sizeof(msg) + sizeof(msg)), sizeof(uint32_t));
  if (st->cmt_idx != cmt_idx)
    return false;

  return true;
}

bool check_outputs(uint8_t *ptr, int node_id) {
  std::unique_ptr<uint8_t[]> creq = std::make_unique<uint8_t[]>(sizeof(msg));
  std::unique_ptr<uint8_t[]> output = std::make_unique<uint8_t[]>(sizeof(msg));
  size_t offset = sizeof(uint32_t) + sizeof(msg);
  ::memcpy(creq.get(), ptr + offset, sizeof(msg));
  offset += sizeof(msg);
  for (auto i = 1ULL; i < node_id; i++) {
    offset += sizeof(msg);
    ::memcpy(output.get(), ptr + offset, sizeof(msg));
    if (::memcmp(creq.get(), output.get(), sizeof(msg)) != 0)
      fmt::print("{} error here\n", __func__);
  }
  return true;
}

/* data is the signed message */
bool verify_execution(char *data, int node_id, state *st) {

  std::tuple<bool, std::unique_ptr<uint8_t[]>> result =
      msg_manager::verify(reinterpret_cast<uint8_t *>(data));

  if (!std::get<0>(result))
    fmt::print("{} verification failed\n", __func__);
  if (check_leader(std::get<1>(result).get(), st)) {
    fmt::print("{} checking leader failed\n", __func__);
  }
  if (check_outputs(std::get<1>(result).get(), node_id))
    return true;
  fmt::print("{} check outputs failed .. \n");
  return false;
}
