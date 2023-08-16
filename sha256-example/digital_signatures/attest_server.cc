#include "net/shared.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fmt/format.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include <vector>

constexpr std::string_view usage = "usage: ./server <nb_server_threads> <port>";

// NOLINTNEXTLINE(cert-err58-cpp, concurrency-mt-unsafe,
// cppcoreguidelines-avoid-non-const-global-variables)
hostent *hostip = gethostbyname("localhost");

int reply_socket = -1;
// how many pending connections the queue will hold?
constexpr int backlog = 1024;

void create_communication_pair(int listening_socket) {
  auto *he = hostip;

  std::cout << "HERE1\n";
#if 0
    auto [bytecount, buffer] = secure_recv(listening_socket);
    if (bytecount == 0) {
        fmt::print("Error on {}\n", __func__);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(1);
    }
    std::cout << "HERE2\n";
    int recv_port = -1;
    ::memcpy(& recv_port, buffer.get(), sizeof(int));
    fmt::print("done here .. {}\n", recv_port);;
#endif
  // TODO: port = take the string
  int port = 30500;

  int sockfd = -1;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fmt::print("socket {}\n", std::strerror(errno));
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(1);
  }

  // connector.s address information
  sockaddr_in their_addr{};
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  their_addr.sin_addr = *(reinterpret_cast<in_addr *>(he->h_addr));
  memset(&(their_addr.sin_zero), 0, sizeof(their_addr.sin_zero));

  bool successful_connection = false;
  for (size_t retry = 0; retry < number_of_connect_attempts; retry++) {
    if (connect(sockfd, reinterpret_cast<sockaddr *>(&their_addr),
                sizeof(struct sockaddr)) == -1) {
      //   	fmt::print("connect {}\n", std::strerror(errno));
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      sleep(1);
    } else {
      successful_connection = true;
      break;
    }
  }
  if (!successful_connection) {
    fmt::print("[{}] could not connect to client after {} attempts ..\n",
               __func__, number_of_connect_attempts);
    exit(1);
  }
  fmt::print("{} {}\n", listening_socket, sockfd);
  reply_socket = sockfd;
}

auto main(int argc, char *argv[]) -> int {
  int port = 18000;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    fmt::print("socket\n");
    exit(1);
  }
  int ret = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) == -1) {
    fmt::print("setsockopt\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(1);
  }

  sockaddr_in my_addr{};
  my_addr.sin_family = AF_INET;         // host byte order
  my_addr.sin_port = htons(port);       // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), 0,
         sizeof(my_addr.sin_zero)); // zero the rest of the struct

  if (bind(sockfd, reinterpret_cast<sockaddr *>(&my_addr), sizeof(sockaddr)) ==
      -1) {
    fmt::print("bind\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(1);
  }

  if (listen(sockfd, backlog) == -1) {
    fmt::print("listen\n");
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    exit(1);
  }

  socklen_t sin_size = sizeof(sockaddr_in);
  fmt::print("waiting for new connections ..\n");
  sockaddr_in their_addr{};

  auto new_fd = accept4(sockfd, reinterpret_cast<sockaddr *>(&their_addr),
                        &sin_size, SOCK_CLOEXEC);
  if (new_fd == -1) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    fmt::print("accecpt() failed ..{}\n", std::strerror(errno));
    exit(1);
  }

  fmt::print("Received request from Client: {}:{}\n",
             inet_ntoa(their_addr.sin_addr), // NOLINT(concurrency-mt-unsafe)
             port);
  fcntl(new_fd, F_SETFL, O_NONBLOCK);
  create_communication_pair(new_fd);
  for (;;) {
    auto [bytecount, buffer] = secure_recv(new_fd);
    if (static_cast<int>(bytecount) <= 0) {
      // TODO: do some error handling here
    }

    // TODO: auto ptr = attest(bytecount, buffer);
    for (auto i = 0ULL; i < bytecount; i++) {
      fmt::print("{}", static_cast<int>(buffer.get()[i]));
    }
    fmt::print("\n");

    auto signature_sz = 256;
    auto ptr =
        std::make_unique<char[]>(bytecount + signature_sz + length_size_field);
    construct_message(ptr.get(), ptr.get(), (bytecount + signature_sz));
    auto actual_sz = destruct_message(ptr.get(), length_size_field);
    std::cout << "size=" << (bytecount + signature_sz)
              << " actual_sz=" << *actual_sz
              << " to reply_socket = " << reply_socket << "\n";
    secure_send(reply_socket, ptr.get(),
                (bytecount + signature_sz + length_size_field));
  }

  return 0;
}
