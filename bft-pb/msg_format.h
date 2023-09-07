#include <cstring>
#include <iostream>
#include <memory>
#include <stdio.h>

struct p_metadata {
  static constexpr size_t HashSize = 32; /* we use SHA256() */
  uint32_t cnt;                          // counter for serialization

  uint8_t prev_h[HashSize];

  /* the size of the cluster is hardcoded */
  uint8_t f1_prev[HashSize];
  uint8_t f2_prev[HashSize];
};

struct p_msg {
  /*
   * Cmd | Output | metadata
   */
  static constexpr size_t CmdSize = 16;
  static constexpr size_t OutSize = 16;

  uint8_t cmd[CmdSize];
  uint8_t output[OutSize];
  p_metadata meta;
};

struct ack_msg {
  /*
   * Cmd | Output | metadata
   */
  static constexpr size_t AckSize = sizeof(bool);
  static constexpr size_t OutSize = 16;
  static constexpr size_t HashSize = 32; /* we use SHA256() */

  uint8_t ack[AckSize];
  uint8_t output[OutSize];
  uint32_t cmt;
  uint32_t sender;
  uint8_t state[HashSize];
};

static int f_get_msg_buf_sz() { return sizeof(ack_msg); }

uint8_t *f_get_output(uint8_t *buf) { return (buf + ack_msg::AckSize); }
uint8_t *f_get_state(uint8_t *buf) {
  return (f_get_output(buf) + sizeof(ack_msg::cmt) + sizeof(ack_msg::sender));
}

uint32_t f_get_sender(uint8_t *buf) {
  int sender = -1;
  auto *ptr = buf + ack_msg::AckSize + ack_msg::OutSize + sizeof(ack_msg::cmt);
  ::memcpy(&sender, ptr, sizeof(uint32_t));
  return sender;
}

bool f_get_ack_val(uint8_t *buf) {
  bool response = false;
  ::memcpy(&response, buf, ack_msg::AckSize);
  return response;
}

uint32_t f_get_cmt(uint8_t *buf) {
  int cmt = 0;
  auto *ptr = buf + ack_msg::AckSize + ack_msg::OutSize;
  ::memcpy(&cmt, ptr, sizeof(uint32_t));
  return cmt;
}

uint8_t *f_get_ack(uint8_t *buf) { return (buf); }

static int p_get_msg_buf_sz() { return sizeof(p_msg); }

uint8_t *p_get_cmd(uint8_t *buf) { return buf; }

uint8_t *p_get_output(uint8_t *buf) { return (buf + p_msg::OutSize); }

std::unique_ptr<p_msg> memcpy_into_p_msg(uint8_t *buf) {
  std::unique_ptr<p_msg> ptr_msg = std::make_unique<p_msg>();

  int offset = 0;
  ::memcpy(ptr_msg->cmd, buf + offset, p_msg::CmdSize);
  offset += p_msg::CmdSize;
  ::memcpy(ptr_msg->output, buf + offset, p_msg::OutSize);
  offset += p_msg::OutSize;
  ::memcpy(&ptr_msg->meta.cnt, buf + offset, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  ::memcpy(ptr_msg->meta.prev_h, buf + offset, p_metadata::HashSize);
  offset += p_metadata::HashSize;
  ::memcpy(ptr_msg->meta.f1_prev, buf + offset, p_metadata::HashSize);
  offset += p_metadata::HashSize;
  ::memcpy(ptr_msg->meta.f2_prev, buf + offset, p_metadata::HashSize);
  return std::move(ptr_msg);
}

uint8_t *p_get_prev_h(uint8_t *buf) {
  int offset = 0;
  offset += p_msg::OutSize;
  offset += p_msg::CmdSize;
  offset += sizeof(uint32_t);
  return (buf + offset);
}

uint8_t *p_get_prev_f1(uint8_t *buf) {
  return (p_get_prev_h(buf) + p_metadata::HashSize);
}

uint8_t *p_get_prev_f2(uint8_t *buf) {
  return (p_get_prev_f1(buf) + p_metadata::HashSize);
}

void memcpy_p_msg_into_buffer(p_msg &msg, uint8_t *buf) {
  // assumes buf allocated correctly
  int offset = 0;
  ::memcpy(buf + offset, msg.cmd, p_msg::CmdSize);
  offset += p_msg::CmdSize;
  ::memcpy(buf + offset, msg.output, p_msg::OutSize);
  offset += p_msg::OutSize;
  ::memcpy(buf + offset, &(msg.meta.cnt), sizeof(uint32_t));
  offset += sizeof(uint32_t);
  ::memcpy(buf + offset, msg.meta.prev_h, p_metadata::HashSize);
  offset += p_metadata::HashSize;
  ::memcpy(buf + offset, msg.meta.f1_prev, p_metadata::HashSize);
  offset += p_metadata::HashSize;
  ::memcpy(buf + offset, msg.meta.f2_prev, p_metadata::HashSize);
}

inline uint32_t p_get_cnt(uint8_t *meta) {
  uint32_t cnt = 0;
  ::memcpy(&cnt, meta, sizeof(uint32_t));
  return cnt;
}

void memcpy_ack_into_buffer(ack_msg &msg, uint8_t *buf) {
  // assumes buf allocated correctly
  int offset = 0;
  ::memcpy(buf + offset, msg.ack, ack_msg::AckSize);
  offset += ack_msg::AckSize;
  ::memcpy(buf + offset, msg.output, ack_msg::OutSize);
  offset += sizeof(uint32_t);
  ::memcpy(buf + offset, &(msg.cmt), sizeof(uint32_t));
  offset += sizeof(uint32_t);
  ::memcpy(buf + offset, &(msg.sender), sizeof(uint32_t));
}
