#pragma once
#include "enc_lib.h"
static const int signature_size = 256;

bool sign_msg(uint8_t *plain_text, size_t plain_text_sz, uint8_t *private_key,
              uint8_t *signed_msg_buf) {
  uint8_t *signature = new uint8_t[signature_size];
  const char *data = reinterpret_cast<char *>(plain_text);
  int encrypted_length =
      priv_sign(data, plain_text_sz, private_key, signature, signature_size);
  if (encrypted_length == -1) {
    fmt::print("[{}] ERROR\n", __func__);
    return false;
  }
  if (encrypted_length != signature_size) {
    fmt::print("[{}] ERROR\n", __func__);
    return false;
  }
  size_t offset = 0;
  fmt::print("[{}] copying data @offset={}.\n", __func__, offset);
  ::memcpy(signed_msg_buf, signature, signature_size);
  offset += signature_size;
  fmt::print("[{}] copying data @offset={}.\n", __func__, offset);
  ::memcpy(signed_msg_buf + offset, &plain_text_sz, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  fmt::print("[{}] copying data @offset={}.\n", __func__, offset);
  ::memcpy(signed_msg_buf + offset, plain_text, plain_text_sz);
  delete[] signature;
  return true;
}

using signed_msg = void *;
bool verify_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
  unsigned char decrypted_hash[sizeof(uint32_t)];
  uint8_t signature[signature_size];
  ::memcpy(signature, signed_msg_buff, signature_size);
  int decrypted_length =
      pub_verify(signature, signature_size, public_key, decrypted_hash);
  uint64_t msg_size_;
  ::memcpy(&msg_size_, signed_msg_buff + signature_size, sizeof(uint64_t));
  const char *payload = reinterpret_cast<char *>(signed_msg_buff) +
                        (signature_size + sizeof(uint64_t));
  for (auto i = 0; i < msg_size_; i++) {
  }
  uint32_t dec_hash;
  ::memcpy(&dec_hash, decrypted_hash, sizeof(uint32_t));
  fmt::print("{} {}\n", dec_hash, CityHash32(payload, msg_size_));
  if (dec_hash == CityHash32(payload, msg_size_))
    return true;
  return false;
}
