#include "log.h"
#include <iostream>
#include <stdlib.h>

int main(void *) {
  std::unique_ptr<trusted_log<16>> log =
      std::make_unique<trusted_log<16>>(1024);
  std::vector<char> data;
  for (auto i = 0ULL; i < 16; i++) {
    data.push_back(static_cast<char>(i));
  }
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  log->a2m_append(data);
  std::cout << "#####\n";
  return 0;
}
