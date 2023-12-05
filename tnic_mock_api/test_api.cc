#include "api.h"
#include <iostream>
#include <stdlib.h>

constexpr int nb_iterations = 10e6;
constexpr int data_sz = 128;
int main(void *) {
  std::vector<uint8_t> data;
  for (auto i = 0ULL; i < data_sz; i++) {
    data.push_back(static_cast<uint8_t>(i));
  }
  for (auto i = 0ULL; i < nb_iterations; i++) {
    auto res = tnic_api::native_get_attestation(data.data(), data.size());
    auto hmac = std::get<0>(res);
    auto att = tnic_api::native_get_attestation(data.data(), data.size());
    auto exp_hmac = std::get<0>(res);
    auto sz = std::get<1>(res);
    if (::memcmp(hmac.data(), exp_hmac.data(), sz) != 0) {
      std::cout << "Error\n";
    }
  }
  std::cout << "#####\n";
  return 0;
}
