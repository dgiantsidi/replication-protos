#pragma once
#include "crypto/hmac_lib.h"
#include <chrono>
#include <thread>

class tnic_api {
public:
  static std::tuple<std::vector<unsigned char>, size_t>
  native_get_attestation(const unsigned char *data, size_t sz) {
    return hmac_sha256(data, sz);
  }

  static std::tuple<std::vector<unsigned char>, size_t>
  SGX_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(50us); //@dimitra: from microbenchmarking

    return hmac_sha256(data, sz);
  }

  AMDSEV_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(50us); //@dimitra: from microbenchmarking

    return hmac_sha256(data, sz);
  }

  static std::tuple<std::vector<unsigned char>, size_t>
  FPGA_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(50us); //@dimitra: from microbenchmarking

    return hmac_sha256(data, sz);
  }
};
