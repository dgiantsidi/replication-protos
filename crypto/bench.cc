#include "hmac_lib.h"
#include "rsa_lib.h"
#include "signed_msg.h"
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
DEFINE_uint64(reps, 10000, "repetitions");

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

  auto hmac_per = [&encrypted, &decrypted](char *plainText, size_t msg_size,
                                           size_t encrypted_length) -> bool {
    const unsigned char *data = reinterpret_cast<unsigned char *>(plainText);
    auto res = hmac_sha256(data, msg_size);
    if (std::get<0>(res).size() != 0 && std::get<1>(res) > 0) {
      return true;
    }
    return false;
  };

  auto verify_per = [&encrypted, &decrypted](char *plainText, size_t msg_size,
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
      // fmt::print("[{}] ERROR: hashes do not match\n", __PRETTY_FUNCTION__);
      return false;
    }
    return true;
  };

  std::string key = "NE1YQmdkUG5HUkJVaUVpR0FYUUpDbXl5em90MGtHMjY="; // ==
  // 4MXBgdPnGRBUiEiGAXQJCmyyzot0kG26
  std::string iv = "b3V5UzVDdkxIQXFVaVdBaE9tZzNmcHJQMzVTVjFTZGg="; // ==
  // ouyS5CvLHAqUiWAhOmg3fprP35SV1Sdh

  auto bench_rsa_version = [&](char *plainText, size_t msg_size) -> bool {
    std::unique_ptr<uint8_t[]> result_buf = std::make_unique<uint8_t[]>(
        RSA_N::signature_size + sizeof(uint64_t) + msg_size);
    auto k = RSA_N::sign_msg(reinterpret_cast<uint8_t *>(plainText), msg_size,
                             reinterpret_cast<uint8_t *>(privateKey),
                             result_buf.get());
    if (!k)
      fmt::print("[{}] error\n", __func__);
    auto m = RSA_N::verify_msg(result_buf.get(),
                               reinterpret_cast<uint8_t *>(publicKey));
    if (!m)
      fmt::print("[{}] error\n", __func__);
    return true;
  };
  std::string dec_str;
  auto bench_hmac_version = [&](char *plainText, size_t msg_size) -> bool {
    std::unique_ptr<uint8_t[]> result_buf = std::make_unique<uint8_t[]>(
        HMAC_N::_max_hash_sz + sizeof(uint64_t) + msg_size);
    auto k = HMAC_N::sign_msg(reinterpret_cast<uint8_t *>(plainText), msg_size,
                              reinterpret_cast<uint8_t *>(privateKey),
                              result_buf.get());
    if (!k)
      fmt::print("[{}] error\n", __func__);
    auto m = HMAC_N::verify_msg(result_buf.get(),
                                reinterpret_cast<uint8_t *>(publicKey));
    if (!m)
      fmt::print("[{}] error\n", __func__);
    return true;
  };

  auto start1 = std::chrono::high_resolution_clock::now();
  int encrypted_length = -1;
  for (auto i = 0ULL; i < reps; i++) {
    encrypted_length = sign_per(reinterpret_cast<char *>(plainText), msg_size);
  }
  auto end1 = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < encrypted_length; i++) {
    fmt::print("{}", encrypted[i]);
  }
  fmt::print("\n");
  auto start2 = std::chrono::high_resolution_clock::now();
  bool res = true;
  for (auto i = 0ULL; i < reps; i++) {
    res &= verify_per(reinterpret_cast<char *>(plainText), msg_size,
                      encrypted_length);
  }
  auto end2 = std::chrono::high_resolution_clock::now();

  auto start3 = std::chrono::high_resolution_clock::now();
  for (auto i = 0ULL; i < reps; i++) {
    res &= hmac_per(reinterpret_cast<char *>(plainText), msg_size,
                    encrypted_length);
  }
  auto end3 = std::chrono::high_resolution_clock::now();

  auto start4 = std::chrono::high_resolution_clock::now();
  for (auto i = 0ULL; i < reps; i++) {
    res &= bench_rsa_version(reinterpret_cast<char *>(plainText), msg_size);
  }
  auto end4 = std::chrono::high_resolution_clock::now();

  auto start5 = std::chrono::high_resolution_clock::now();
  for (auto i = 0ULL; i < reps; i++) {
    res &= bench_hmac_version(reinterpret_cast<char *>(plainText), msg_size);
  }
  auto end5 = std::chrono::high_resolution_clock::now();

  // TODO: take time
  if (res)
    fmt::print("ola kala!\n");
  for (auto i = 0; i < max_hash_sz; i++) {
    fmt::print("{}", decrypted[i]);
  }
  fmt::print("\n");

  auto elapsed1 = end1 - start1;
  auto elapsed2 = end2 - start2;
  auto elapsed3 = end3 - start3;
  auto elapsed4 = end4 - start4;
  auto elapsed5 = end5 - start5;
  long long avg_duration1 =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed1).count();
  long long avg_duration2 =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed2).count();
  long long avg_duration3 =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed3).count();
  long long avg_duration4 =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed4).count();
  long long avg_duration5 =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed5).count();

  std::cout << "elapsed time: " << avg_duration1 << " us for " << reps
            << " rsa+sha256 (sign)" << std::endl;
  std::cout << "elapsed time: " << avg_duration2 << " us for " << reps
            << " rsa+sha256 (verify)" << std::endl;
  std::cout << "elapsed time: " << avg_duration3 << " us for " << reps
            << " hmac-sha256" << std::endl;
  std::cout << "elapsed time: " << avg_duration4 << " us for " << reps
            << " rsa_sign_msg" << std::endl;
  std::cout << "elapsed time: " << avg_duration5 << " us for " << reps
            << " hmac_sign_msg" << std::endl;

  delete[] encrypted;
  delete[] decrypted;
  delete[] plainText;
  return 0;
}
