// #include "signed_msg.h"
#include "enc_lib.h"
#include <chrono>
#include <cstring>
#include <gflags/gflags.h>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

DEFINE_uint64(msg_size, 64, "Size of request message in bytes");
DEFINE_uint64(reps, 10, "repetitions");

static const int signature_size = 256; // RSA_size(rsa);

int main(int args, char *argv[]) {
  gflags::ParseCommandLineFlags(&args, &argv, true);
  auto msg_size = FLAGS_msg_size;
  auto reps = FLAGS_reps;

  uint8_t *plainText = new uint8_t[msg_size];
  for (auto i = 0ULL; i < msg_size; i++) {
    plainText[i] = 'd';
  }
  uint8_t *encrypted = new uint8_t[signature_size];
  uint8_t *decrypted = new uint8_t[max_hash_sz];

  auto sign_per = [&encrypted](char *plainText, size_t msg_size) -> int {
    int encrypted_length = priv_sign_sha256(
        reinterpret_cast<char *>(plainText), msg_size,
        reinterpret_cast<uint8_t *>(privateKey), encrypted, signature_size);
    if (encrypted_length == -1) {
      fmt::print("[{}] signing w/ priv key failed.\n", __PRETTY_FUNCTION__);
      exit(0);
    }
    return encrypted_length;
  };

  auto dec_per = [&encrypted, &decrypted](char *plainText, size_t msg_size,
                                          size_t encrypted_length) -> bool {
    int decrypted_length =
        pub_verify_sha256(encrypted, encrypted_length,
                          reinterpret_cast<uint8_t *>(publicKey), decrypted);

    if (decrypted_length == -1) {
      fmt::print("[{}] decrypting w/ pub key failed.\n", __PRETTY_FUNCTION__);
      exit(0);
    }
    uint8_t decrypted_hash[max_hash_sz];
    ::memcpy(&decrypted_hash, decrypted, max_hash_sz);

    auto computed_hash =
        get_sha256(reinterpret_cast<char *>(plainText), msg_size);
    if (::memcmp(computed_hash.get(), decrypted_hash, max_hash_sz) != 0) {
      fmt::print("[{}] ERROR: hashes do not match\n", __PRETTY_FUNCTION__);
      return false;
    }
    return true;
  };

  auto start1 = std::chrono::system_clock::now();
  int encrypted_length = -1;
  for (auto i = 0ULL; i < reps; i++) {
    encrypted_length = sign_per(reinterpret_cast<char *>(plainText), msg_size);
  }
  auto end1 = std::chrono::system_clock::now();
  for (auto i = 0; i < encrypted_length; i++) {
    fmt::print("{}", encrypted[i]);
  }
  fmt::print("\n");
  auto start2 = std::chrono::system_clock::now();
  bool res = true;
  for (auto i = 0ULL; i < reps; i++) {
    res &= dec_per(reinterpret_cast<char *>(plainText), msg_size,
                   encrypted_length);
  }
  auto end2 = std::chrono::system_clock::now();
  // TODO: take time
  if (res)
    fmt::print("ola kala!\n");
  for (auto i = 0; i < max_hash_sz; i++) {
    fmt::print("{}", decrypted[i]);
  }
  fmt::print("\n");

  std::chrono::duration<double> elapsed_seconds1 = end1 - start1;
  std::chrono::duration<double> elapsed_seconds2 = end2 - start2;
  std::cout << "elapsed time: " << elapsed_seconds1.count() << "s for " << reps
            << " rsa+sha256 (sign)" << std::endl;
  std::cout << "elapsed time: " << elapsed_seconds2.count() << "s for " << reps
            << " rsa+sha256 (verify)" << std::endl;

  delete[] encrypted;
  delete[] decrypted;
  delete[] plainText;
  return 0;
}
