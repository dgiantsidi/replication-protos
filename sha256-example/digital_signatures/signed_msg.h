#pragma once
#include "enc_lib.h"
static const int signature_size = 256; // RSA_size(rsa);

bool sign_msg(uint8_t *plain_text, size_t plain_text_sz, uint8_t *private_key,
              uint8_t *signed_msg_buf) {

  uint8_t *signature = new uint8_t[signature_size];
  const char *data = reinterpret_cast<char *>(plain_text);
  int encrypted_length = priv_sign_sha256(data, plain_text_sz, private_key,
                                          signature, signature_size);

  if (encrypted_length == -1) {
    fmt::print("[{}] ERROR\n", __func__);
    return false;
  }
  if (encrypted_length != signature_size) {
    fmt::print("[{}] ERROR\n", __func__);
    return false;
  }
  size_t offset = 0;
#ifndef PRINT_DEBUG
  fmt::print("[{}] copying data @offset={}.\n", __func__, offset);
#endif
  ::memcpy(signed_msg_buf, signature, signature_size);
  offset += signature_size;
#ifndef PRINT_DEBUG
  fmt::print("[{}] copying data @offset={} (plain_text_sz={}).\n", __func__,
             offset, plain_text_sz);
#endif
  ::memcpy(signed_msg_buf + offset, &plain_text_sz, sizeof(uint64_t));
  offset += sizeof(uint64_t);
#ifndef PRINT_DEBUG
  fmt::print("[{}] copying data plain_text_sz={} @offset={} plain text.\n",
             __func__, plain_text_sz, offset);
  for (auto i = 0; i < plain_text_sz; i++) {
    fmt::print("{}", plain_text[i]);
  }
  fmt::print("\n");
#endif
  ::memcpy(signed_msg_buf + offset, plain_text, plain_text_sz);

  fmt::print("[{}] HERE IS THE FINAL THING (total_size={}) \n", __func__,
             (offset + plain_text_sz));
  for (auto i = 0; i < (offset + plain_text_sz); i++) {
    fmt::print("{}", signed_msg_buf[i]);
  }
  fmt::print("[{}] END\n", __func__);

  delete[] signature;
  return true;
}

using signed_msg = void *;
bool verify_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
#ifdef PRINT_DEBUG
  fmt::print("[{}] #1\n", __func__);
#endif
  unsigned char decrypted_hash[max_hash_sz];
  uint8_t signature[signature_size];
  ::memcpy(signature, signed_msg_buff, signature_size);
  int decrypted_length =
      pub_verify_sha256(signature, signature_size, public_key, decrypted_hash);
  uint64_t msg_size_;
  ::memcpy(&msg_size_, signed_msg_buff + signature_size, sizeof(uint64_t));
  const char *payload = reinterpret_cast<char *>(signed_msg_buff) +
                        (signature_size + sizeof(uint64_t));
  for (auto i = 0; i < msg_size_; i++) {
  }
  unsigned char dec_hash[max_hash_sz];
  ::memcpy(dec_hash, decrypted_hash, max_hash_sz);
  auto a = get_sha256(payload, msg_size_);
  // FIXME
  if (::memcmp(a.get(), dec_hash, max_hash_sz) == 0) {
    fmt::print("[{}] correct\n", __func__);
    return true;
  }

  return false;
}

std::tuple<int, std::unique_ptr<uint8_t[]>>
verify_get_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
#ifndef PRINT_DEBUG
  fmt::print("[{}] #1\n", __func__);
#endif
  unsigned char decrypted_hash[max_hash_sz];
  uint8_t signature[signature_size];
  ::memcpy(signature, signed_msg_buff, signature_size);
  int decrypted_length =
      pub_verify_sha256(signature, signature_size, public_key, decrypted_hash);
#ifndef PRINT_DEBUG
  fmt::print("[{}] decrypted_length={} #2\n", __func__, decrypted_length);
#endif
  uint64_t msg_size_;
  ::memcpy(&msg_size_, signed_msg_buff + signature_size, sizeof(uint64_t));
  const char *payload = reinterpret_cast<char *>(signed_msg_buff) +
                        (signature_size + sizeof(uint64_t));
#ifndef PRINT_DEBUG
  fmt::print("[{}] msg_size_={} #3 payload=\n", __func__, msg_size_);
#endif
  for (auto i = 0; i < msg_size_; i++) {
    fmt::print("{}", payload[i]);
  }
  fmt::print(">> \n");
  unsigned char dec_hash[max_hash_sz];
  ::memcpy(dec_hash, decrypted_hash, max_hash_sz);

  auto data_payload = std::make_unique<uint8_t[]>(msg_size_);
  auto a = get_sha256(payload, msg_size_);
  // FIXME
  if (::memcmp(a.get(), dec_hash, max_hash_sz) == 0) {
    fmt::print("[{}] correct\n", __func__);
    ::memcpy(data_payload.get(),
             (signed_msg_buff + (signature_size + sizeof(uint64_t))),
             msg_size_);
    for (auto i = 0; i < msg_size_; i++) {
      fmt::print("{}", data_payload.get()[i]);
    }
    fmt::print(">> \n");

    return std::make_tuple(msg_size_, std::move(data_payload));
  }
  return std::make_tuple(-1, std::move(data_payload));
}
