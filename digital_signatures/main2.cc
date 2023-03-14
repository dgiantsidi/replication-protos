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
  size_t alloc_sz = (signature_size * sizeof(uint8_t) + sizeof(uint64_t) +
                     1024 * sizeof(uint8_t)) /
                    sizeof(uint8_t);
  fmt::print("[{}] alloc {} bytes\n", __func__, alloc_sz);
  auto *buff = new uint8_t[alloc_sz];

  char plainText[1024];
  for (auto i = 0ULL; i < 1024; i++) {
    plainText[i] = 'd';
  }

  // TODO: sign msg (input the plaintext and a buff and output the buff signed)
  bool ret = sign_msg(reinterpret_cast<uint8_t *>(plainText), 1024,
                      reinterpret_cast<uint8_t *>(privateKey), buff);
  if (!ret) {
    fmt::print("[{}] Private Encrypt failed.\n", __PRETTY_FUNCTION__);
    exit(0);
  }

  // TODO: verify the signed message (input a signed message (buff) and output
  // true false)
  ret = verify_msg(buff, reinterpret_cast<uint8_t *>(publicKey));
  if (!ret) {

    fmt::print("[{}] Public Decrypt failed.\n", __PRETTY_FUNCTION__);
    exit(0);
  }
  delete[] buff;
  return 0;
}
