#include <chrono>
#include <fmt/printf.h>

/* the batched message */
struct msg_manager {
  static constexpr size_t batch_count = 16;
  msg_manager() {
    buffer = std::make_unique<uint8_t[]>(batch_count * kMsgSize);
    fmt::print("sizeof(msg)={}\n", kMsgSize);
  };

  bool enqueue_req(uint8_t *buf, size_t buf_sz) {
    if (buf_sz != kMsgSize)
      fmt::print("[{}] buf_sz ({}) != sizeof(msg) ({})\n", __PRETTY_FUNCTION__,
                 buf_sz, kMsgSize);
    if (cur_idx < batch_count) {
      ::memcpy(buffer.get() + cur_idx * kMsgSize, buf, buf_sz);
      cur_idx++;
      return true;
    } else
      return false;
  }

  uint8_t *serialize_batch() { return buffer.get(); }

  void empty_buff() {
    // fmt::print("{}\n", __func__);
    cur_idx = 0;
  }

  static std::unique_ptr<uint8_t[]> deserialize(uint8_t *buf, size_t buf_sz) {
    if (buf_sz != kMsgSize * batch_count) {
      fmt::print("[{}] buf_sz ({}B) != batch_count*sizeof(msg) ({}B)\n",
                 __PRETTY_FUNCTION__, buf_sz, (kMsgSize * batch_count));
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10000ms);
    }
    auto data = std::make_unique<uint8_t[]>(buf_sz);
    ::memcpy(data.get(), buf, buf_sz);
    return std::move(data);
  }

  std::unique_ptr<uint8_t[]> buffer;
  uint32_t cur_idx = 0;
};
