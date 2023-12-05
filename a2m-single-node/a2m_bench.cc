#include "log.h"
#include <chrono>
#include <fmt/printf.h>
#include <iostream>
#include <stdlib.h>

int main(void *) {
  constexpr size_t CtxSize = 64; // value size in B
  constexpr int iterations = 100e6;
  constexpr size_t LogSize =
      (trusted_log<CtxSize>::log_entry::HashSize +
       trusted_log<CtxSize>::log_entry::CtxSize +
       sizeof(trusted_log<CtxSize>::log_entry::sequencer)) *
      iterations;
  std::unique_ptr<trusted_log<CtxSize>> log =
      std::make_unique<trusted_log<CtxSize>>(LogSize);
  std::vector<char> data;
  for (auto i = 0ULL; i < CtxSize; i++) {
    data.push_back(static_cast<char>(i));
  }
  {
    auto start = std::chrono::steady_clock::now();
    for (auto i = 0ULL; i < iterations; i++) {
      log->a2m_append(data);
      if (i % 200000 == 0)
        fmt::print("appends={}\n", i);
    }
    auto end = std::chrono::steady_clock::now();
    auto duration2 =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    const std::chrono::duration<double> elapsed_seconds{end - start};
    fmt::print("[{}] elapsed={}/{} for {} iterations (avg_lat={} sec\t {}us)\n",
               __PRETTY_FUNCTION__, elapsed_seconds.count(), duration2.count(),
               iterations, (elapsed_seconds.count() * 1.0) / (iterations * 1.0),
               ((duration2.count() * 1.0) / (iterations * 1.0)));
  }

  {
    auto start = std::chrono::steady_clock::now();
    for (auto i = 0ULL; i < iterations; i++) {
      auto [[maybe_unused]] ret = log->a2m_lookup(i, i);
      if (ret.e.sequencer != i) {
        fmt::print("[{}] ret.e.sequencer={} != i={}\n", __PRETTY_FUNCTION__,
                   ret.e.sequencer, i);
      }
      if (i % 200000 == 0)
        fmt::print("lookups={}\n", i);
    }
    auto end = std::chrono::steady_clock::now();
    auto duration2 =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    const std::chrono::duration<double> elapsed_seconds{end - start};
    fmt::print("[{}] elapsed={} for {} iterations (avg_lat={} sec\t {}us)\n",
               __PRETTY_FUNCTION__, elapsed_seconds.count(), iterations,
               (elapsed_seconds.count() * 1.0) / (iterations * 1.0),
               (duration2.count() * 1.0) / (iterations * 1.0));
  }
  return 0;
}
