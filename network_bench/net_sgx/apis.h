#include "shared.h"
#include <fmt/printf.h>

class authenticator {
private:
  const int port = 18000;
  hostent *hostip = nullptr;
  int sending_fd = -1;
  int fd = -1;

public:
  std::pair<int, std::unique_ptr<char[]>>
  get_attestation_from_sgx(int size, const char *data) {
    auto sending_sz = size + length_size_field;
    auto buff = std::make_unique<char[]>(sending_sz);

    construct_message(buff.get(), data, size);
    sent_request(buff.get(), sending_sz, sending_fd);
    auto res = recv_ack(fd);
    auto bytecount = std::get<0>(res);
    auto ptr = std::move(std::get<1>(res));

    return std::make_pair(bytecount, std::move(ptr));
  };

  authenticator() {
    hostip = gethostbyname("localhost");
    fd = connect_to_the_server(port, "localhost", sending_fd);

    fmt::print("[{}] connect_to_the_server sending_fd={} fd={}\n", __func__,
               sending_fd, fd);
  }
};
