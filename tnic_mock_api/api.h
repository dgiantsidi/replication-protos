#pragma once
#include "crypto/hmac_lib.h"
#include <chrono>
#include <thread>

class tnic_api {
public:
  static int
  time_elapsed(const std::chrono::time_point<std::chrono::steady_clock> start) {
    auto cur_time = std::chrono::steady_clock::now();
    auto duration_ =
        std::chrono::duration_cast<std::chrono::microseconds>(cur_time - start);
    return duration_.count();
#if 0
				while (duration_.count() < 5) {
					cur_time = std::chrono::steady_clock::now();
					duration_ =  std::chrono::duration_cast<std::chrono::microseconds>(cur_time - start);
				}
#endif
  }
  static std::tuple<std::vector<unsigned char>, size_t>
  native_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::chrono_literals;
    //			std::this_thread::sleep_for(10us); //@dimitra: from
    // microbenchmarking
#if 1
    constexpr int native_delay_in_us = 10;

    auto start = std::chrono::steady_clock::now();
    while (time_elapsed(start) < native_delay_in_us) {
    }
#endif
    return hmac_sha256(data, sz);
  }

  static std::tuple<std::vector<unsigned char>, size_t>
  SGX_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::literals::chrono_literals;
    // std::this_thread::sleep_for(45us); //@dimitra: from microbenchmarking
    constexpr int sgx_delay_in_us = 45;

    using namespace std::literals::chrono_literals;
    auto start = std::chrono::steady_clock::now();
    while (time_elapsed(start) < sgx_delay_in_us) {
    }

    return hmac_sha256(data, sz);
  }

  static std::tuple<std::vector<unsigned char>, size_t>
  AMDSEV_get_attestation(const unsigned char *data, size_t sz) {
    using namespace std::literals::chrono_literals;
    // std::this_thread::sleep_for(std::chrono::microseconds(5)); //@dimitra:
    // from microbenchmarking
    constexpr int amdsev_delay_in_us = 31;

    using namespace std::literals::chrono_literals;
    auto start = std::chrono::steady_clock::now();
    while (time_elapsed(start) < amdsev_delay_in_us) {
    }

    return hmac_sha256(data, sz);
  }

  static std::tuple<std::vector<unsigned char>, size_t>
  FPGA_get_attestation(const unsigned char *data, size_t sz) {
    constexpr int fpga_delay_in_us = 7;

    using namespace std::literals::chrono_literals;
    auto start = std::chrono::steady_clock::now();
    while (time_elapsed(start) < fpga_delay_in_us) {
    }

    return hmac_sha256(data, sz);
  }
};
