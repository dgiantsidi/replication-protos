#pragma once
#include <chrono>
#include <fmt/printf.h>

/* message format to populate the request */
struct msg {
  static constexpr size_t key_sz = 8;
  static constexpr size_t value_sz = 40;
  struct header {
    uint32_t src_node;
    uint32_t dest_node;
    uint32_t seq_idx;
    uint32_t msg_size;
  };
  // msg format
  header hdr;
  uint8_t key[key_sz];
  uint8_t value[value_sz];
};

/* message format for acks (for messages with consecutive ids ranging from s_idx
 * to e_idx) */
struct cmt_msg {
  struct header {
    uint32_t src_node;
    uint32_t dest_node;
    uint32_t seq_idx;
    uint32_t msg_size;
  };
  // msg format
  header hdr;
  uint64_t cns_rd; /* consensus round id */
  uint64_t e_idx;
};

/* the batched message */
struct msg_manager {
  static constexpr size_t batch_count = 16;
  using consensus_round = uint64_t;
  static constexpr size_t batch_sz =
      batch_count * sizeof(msg) + sizeof(consensus_round);
  msg_manager() {
    buffer = std::make_unique<uint8_t[]>(batch_count * sizeof(msg) +
                                         sizeof(consensus_round));
    ::memcpy(buffer.get(), &count, sizeof(count));
    fmt::print("[{}] sizeof(msg)={}, sizeof(consensus_round)={}\n",
               __PRETTY_FUNCTION__, sizeof(msg), sizeof(consensus_round));
  };

  consensus_round get_consensus_round(uint8_t *buff) {
    consensus_round rd;
    ::memcpy(&rd, buff, sizeof(consensus_round));
    return rd;
  }

  bool enqueue_req(uint8_t *buf, size_t buf_sz) {
    if (buf_sz != sizeof(msg))
      fmt::print("[{}] buf_sz != sizeof(msg)\n", __PRETTY_FUNCTION__);
    if (cur_idx < batch_count) {
      ::memcpy(buffer.get() + sizeof(count) + cur_idx * sizeof(msg), buf,
               buf_sz);
      cur_idx++;
      return true;
    } else
      return false;
  }

  uint8_t *serialize_batch() { return buffer.get(); }

  void empty_buff() {
    cur_idx = 0;
    count++;
    ::memcpy(buffer.get(), &count, sizeof(count));
  }

  static std::unique_ptr<uint8_t[]> deserialize(uint8_t *buf, size_t buf_sz) {
    if (buf_sz != sizeof(msg) * batch_count + sizeof(count)) {
      fmt::print("[{}] buf_sz ({}B) != batch_count*sizeof(msg) ({}B)\n",
                 __PRETTY_FUNCTION__, buf_sz,
                 (sizeof(msg) * batch_count + sizeof(count)));
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10000ms);
    }
    auto data = std::make_unique<uint8_t[]>(buf_sz);
    ::memcpy(data.get(), buf, buf_sz);
    return std::move(data);
  }

  static void print_batched(std::unique_ptr<uint8_t[]> msgs, size_t sz) {
    int min_idx = -1, max_idx = -1;
    for (auto i = 0ULL; i < batch_count; i++) {
      auto msg_data = std::make_unique<msg>();
      ::memcpy(reinterpret_cast<uint8_t *>(msg_data.get()),
               msgs.get() + sizeof(consensus_round) + i * sizeof(msg),
               sizeof(msg::header));
      if (i == 0)
        min_idx = msg_data->hdr.seq_idx;
      if (i == (batch_count - 1))
        max_idx = msg_data->hdr.seq_idx;
    }
    fmt::print("[{}] min_idx={}\tmax_idx={}\n", __PRETTY_FUNCTION__, min_idx,
               max_idx);
  }

  static std::vector<int> parse_indexes(std::unique_ptr<uint8_t[]> msgs,
                                        size_t sz) {
    int min_idx = -1, max_idx = -1, prev_idx = -1;
    size_t msgs_num =
        (sz < batch_count * sizeof(msg)) ? (sz / sizeof(msg)) : batch_count;
    for (auto i = 0ULL; i < msgs_num; i++) {
      auto msg_data = std::make_unique<msg>();
      ::memcpy(reinterpret_cast<uint8_t *>(msg_data.get()),
               msgs.get() + i * sizeof(msg), sizeof(msg::header));

#ifdef PRINT_DEBUG
      fmt::print("[{}] idx={}\n", __func__, msg_data->hdr.seq_idx);
#endif
      if (i == 0) {
        min_idx = msg_data->hdr.seq_idx;
        prev_idx =
            min_idx - 1; // I will increase it later on for this iteration
      } else if (i == (msgs_num - 1))
        max_idx = msg_data->hdr.seq_idx;
      else {
        if ((prev_idx + 1) != msg_data->hdr.seq_idx)
          fmt::print("[{}] an error happenned here .. expected={}\treal={}\n",
                     __func__, (prev_idx + 1), msg_data->hdr.seq_idx);
      }
      prev_idx++;
    }
    return std::vector<int>{min_idx, max_idx};
  }

  std::unique_ptr<uint8_t[]> buffer;
  uint32_t cur_idx = 0;
  consensus_round count = 0;
};
