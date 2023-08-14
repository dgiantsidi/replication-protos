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

  sent_request(buf.get(), size, sending_fd);
  recv_ack(fd);

}

auto main(int argc, char *argv[]) -> int {


  // NOLINTNEXTLINE(concurrency-mt-unsafe)

  auto port = 18000;
  auto sz = 128;
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(sz);
  client(port, std::move(buf), sz);

}

