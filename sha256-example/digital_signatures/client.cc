#include <algorithm>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "net/shared.h"

hostent *hostip = gethostbyname("localhost");

void client(int port, std::unique_ptr<char[]> buf, size_t size) {
  hostip = gethostbyname("localhost");
  std::vector<long long> latencies;
  auto sending_fd = -1;
  auto fd = connect_to_the_server(port, "localhost", sending_fd);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  // sleep(2);
  fmt::print("[{}] connect_to_the_server sending_fd={} fd={}\n", __func__,
             sending_fd, fd);

  auto reqs_num = 1e6;
  uint64_t cnt = 0;
  bool flag_s = false;
  bool flag_e = false;
  auto start = std::chrono::high_resolution_clock::now();
  auto _start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < reqs_num; i++) {
    //   fmt::print("[{}] \n", __func__);
    if (i > 500000 && i < 500300) {
      flag_s = true;
      flag_e = true;
    } else {
      flag_s = false;
      flag_e = false;
    }
    if (flag_s)
      _start = std::chrono::high_resolution_clock::now();
    cnt++;
    auto sending_sz = size + length_size_field;
    auto buff = std::make_unique<char[]>(sending_sz);

    construct_message(buff.get(), buf.get(), size);
    auto actual_msg_size_opt = destruct_message(buff.get(), length_size_field);
    /*
       std::cout << "size = " << size
       << " but actual_msg_size=" << *actual_msg_size_opt << "\n";
       for (auto i = 0ULL; i < sending_sz; i++) {
       std::cout << static_cast<int>(buff.get()[i]);
       }
       std::cout << "\n";
       */
    sent_request(buff.get(), sending_sz, sending_fd);
    // fmt::print("[{}] sent_request\n", __func__);
    recv_ack(fd);
    // fmt::print("[{}] recv_ack\n", __func__);
    if (flag_e) {
      auto elapsed_ = std::chrono::high_resolution_clock::now() - _start;
      long long latency_ =
          std::chrono::duration_cast<std::chrono::microseconds>(elapsed_)
              .count();
      auto avg_lat_ = latency_;
      latencies.push_back(avg_lat_);
      cnt = 0;
    }
  }
  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  long long avg_duration =
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

  std::cout << "RESULT: " << avg_duration << " and latency="
            << static_cast<float>((avg_duration * 1.0) / (reqs_num * 1.0))
            << "us \n";
  for (auto &elem : latencies)
    std::cout << elem << "\n";
  std::cout << "\n";
}

auto main(int argc, char *argv[]) -> int {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  auto port = 18000;
  auto sz = 32;
  // fmt::print("[{}] addr={}\n", __func__, hostip->h_addr);
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(sz);
  //  ::memset(buf.get(), '1', sz);
  client(port, std::move(buf), sz);
}
