#include "signed_msg.h"
#include <cstring>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

int main() {

  // char plainText[2048/8] = "Hello this is Ravi"; //key length : 2048
  int alloc_sz = (signature_size * sizeof(uint64_t) + sizeof(uint64_t) +
                  sizeof(uint8_t) * 1024) /
                 sizeof(uint8_t);
  auto buff = std::make_unique<uint8_t[]>(alloc_sz);

  char plainText[1024];
  for (auto i = 0ULL; i < 1024; i++) {
    plainText[i] = 'd';
  }

  uint8_t *encrypted = new uint8_t[signature_size];
  uint8_t *decrypted = new uint8_t[sizeof(uint32_t)];

  // TODO: sign msg (input the plaintext and a buff and output the buff signed)
  int encrypted_length =
      priv_sign(plainText, 1024, reinterpret_cast<uint8_t *>(privateKey),
                encrypted, signature_size);
  if (encrypted_length == -1) {
    fmt::print("[{}] Private Encrypt failed.\n", __PRETTY_FUNCTION__);
    exit(0);
  }
  fmt::printf("[{}] encrypted length={}\n", __PRETTY_FUNCTION__,
              encrypted_length);
  ::memcpy(buff.get(), encrypted, encrypted_length);
  ::memcpy(buff.get() + encrypted_length + sizeof(uint64_t), plainText, 1024);

  // TODO: verify the signed message (input a signed message (buff) and output
  // true false) int decrypted_length = public_decrypt(encrypted,
  // encrypted_length, reinterpret_cast<uint8_t*>(publicKey), decrypted);
  int decrypted_length =
      pub_verify(encrypted, encrypted_length,
                 reinterpret_cast<uint8_t *>(publicKey), decrypted);
  if (decrypted_length == -1) {
    print_error("Public Decrypt failed");
    exit(0);
  }
  uint32_t decrypted_hash;
  ::memcpy(&decrypted_hash, decrypted, sizeof(decrypted_hash));
  fmt::print("[{}] decrypted text={}\n", __PRETTY_FUNCTION__, decrypted_hash);
  fmt::print("[{}] decrypted length={}\n", __PRETTY_FUNCTION__,
             decrypted_length);
  if (decrypted_hash != CityHash32(plainText, 1024)) {
    fmt::print("[{}] ERROR: hashes do not match\n", __PRETTY_FUNCTION__);
  }

  delete[] encrypted;
  delete[] decrypted;
  return 0;
}
