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
  uint8_t *decrypted = new uint8_t[max_hash_sz];

  // TODO: sign msg (input the plaintext and a buff and output the buff signed)
  int encrypted_length =
      priv_sign_sha256(plainText, 1024, reinterpret_cast<uint8_t *>(privateKey),
                       encrypted, signature_size);
  if (encrypted_length == -1) {
    fmt::print("[{}] Private Encrypt failed.\n", __PRETTY_FUNCTION__);
    exit(0);
  }
  fmt::print("[{}] encrypted length={}\n", __PRETTY_FUNCTION__,
             encrypted_length);
  ::memcpy(buff.get(), encrypted, encrypted_length);
  ::memcpy(buff.get() + encrypted_length + sizeof(uint64_t), plainText, 1024);

  // TODO: verify the signed message (input a signed message (buff) and output
  // true false) int decrypted_length = public_decrypt(encrypted,
  // encrypted_length, reinterpret_cast<uint8_t*>(publicKey), decrypted);
  fmt::print("{} before decryption\n", __func__);
  int decrypted_length =
      pub_verify_sha256(encrypted, encrypted_length,
                        reinterpret_cast<uint8_t *>(publicKey), decrypted);
  fmt::print("{} after decryption\n", __func__);
  if (decrypted_length == -1) {
    print_error("Public Decrypt failed");
    exit(0);
  }
  uint8_t decrypted_hash[max_hash_sz];
  ::memcpy(&decrypted_hash, decrypted, max_hash_sz);

  // fmt::print("[{}] decrypted text={}\n", __PRETTY_FUNCTION__,
  // decrypted_hash);
  // fmt::print("[{}] decrypted length={}\n", __PRETTY_FUNCTION__,
  //           decrypted_length);
  //
  auto computed_hash = get_sha256(plainText, 1024);
  if (::memcmp(computed_hash.get(), decrypted_hash, max_hash_sz) != 0) {
    fmt::print("[{}] ERROR: hashes do not match\n", __PRETTY_FUNCTION__);
  }
  fmt::print("[{}] the do fit!\n", __func__);
  delete[] encrypted;
  delete[] decrypted;
  return 0;
}
