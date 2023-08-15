#include <algorithm>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "net/shared.h"

hostent* hostip = gethostbyname("localhost");

void client(int port, std::unique_ptr<char[]> buf, size_t size) {
  auto sending_fd = -1;
  auto fd = connect_to_the_server(port, "localhost", sending_fd);

  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  // sleep(2);
  fmt::print("[{}] connect_to_the_server()\n", __func__);

  fmt::print("[{}] \n", __func__);
  auto sending_sz = size + length_size_field;
  auto buff = std::make_unique<char[]>(sending_sz + 1);
  buff.get()[sending_sz] = '\0';

  construct_message(buff.get(), buf.get(), size);
  auto actual_msg_size_opt = destruct_message(buff.get(), length_size_field);
  std::cout << *actual_msg_size_opt << "\n";
for (auto i = 0ULL; i < sending_sz; i++) {
	std::cout << static_cast<int>(buff.get()[i]);
}
std::cout <<"\n";
  sent_request(buff.get(), sending_sz, sending_fd);
  fmt::print("[{}] sent_request\n", __func__);
 // recv_ack(fd);
 // fmt::print("[{}] recv_ack\n", __func__);

}

auto main(int argc, char *argv[]) -> int {


  // NOLINTNEXTLINE(concurrency-mt-unsafe)

  auto port = 18000;
  auto sz = 128 + 256;
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(sz);
  client(port, std::move(buf), sz);

}

