#include "crypto/signed_msg.h"
#include <fmt/printf.h>

struct state {
  uint32_t cmt_idx;
};

/* msg_format:
 *      1. original (client) request            (key --- val --- PUT)
 *      2. leader's action/output               (key --- val --- PUT --- cmt)
 *      3. <middle1 output> <middle2 output> <middle3 output>
 */

struct msg {
  static constexpr size_t key_sz = 8;
  static constexpr size_t value_sz = 40 - sizeof(uint8_t);
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
  uint8_t operation;
};

struct cmt_msg {
  struct header {
    uint32_t src_node;
    uint32_t dest_node;
    uint32_t seq_idx;
    uint32_t msg_size;
  };
  // msg format
  header hdr;
  uint64_t s_cmt_idx;
  uint64_t e_cmt_idx;
};

struct msg_manager {
  static constexpr size_t batch_count =
      1; // TODO: validation fails with batching
  static constexpr size_t message_size =
      (sizeof(msg) + sizeof(msg) + sizeof(uint32_t) + 2 * sizeof(msg));
  msg_manager() {
    buffer = std::make_unique<uint8_t[]>(
        batch_count *
        (sizeof(msg) + sizeof(msg) + sizeof(uint32_t) + 2 * sizeof(msg)));
    fmt::print("[{}] INFO: msg={}\tmessage_size={}\n", __PRETTY_FUNCTION__,
               sizeof(msg), message_size);
  };

  bool enqueue_req(uint8_t *buf, size_t buf_sz) {
    if (buf_sz > message_size)
      fmt::print("[{}] ERROR: buf_sz ({}B)!= message_size ({}B)\n",
                 __PRETTY_FUNCTION__, buf_sz, message_size);
    if (cur_idx < batch_count) {
      ::memcpy(buffer.get() + cur_idx * message_size, buf, buf_sz);
      cur_idx++;
      return true;
    } else
      return false;
  }

  std::tuple<size_t, std::unique_ptr<uint8_t[]>> attest() {
    size_t alloc_sz =
        (HMAC_N::signature_size * sizeof(uint8_t) + sizeof(uint64_t) +
         (batch_count * message_size) * sizeof(uint8_t)) /
        sizeof(uint8_t);

#ifdef DEBUG_PRINT
    fmt::print("[{}] alloc {} bytes\n", __func__, alloc_sz);
#endif
    auto buff = std::make_unique<uint8_t[]>(alloc_sz);

    // TODO: assign the counter
    char *privateKey = nullptr;
    bool ret = HMAC_N::sign_msg(
        reinterpret_cast<uint8_t *>(buffer.get()), (batch_count * message_size),
        reinterpret_cast<uint8_t *>(privateKey), buff.get());
    if (!ret) {
      fmt::print("[{}] ERROR: signing w/ priv key failed.\n",
                 __PRETTY_FUNCTION__);
      exit(0);
    }
#ifdef DEBUG_PRINT
    fmt::print("[{}] encryption done ..\n", __func__);
#endif
    return std::make_tuple(alloc_sz, std::move(buff));
  }

  static std::tuple<int, std::unique_ptr<uint8_t[]>> verify(uint8_t *data) {
#ifdef DEBUG_PRINT
    fmt::print("***** {} start *****\n", __func__);
#endif
    char *publicKey = nullptr;
    auto ret =
        HMAC_N::verify_get_msg(data, reinterpret_cast<uint8_t *>(publicKey));
#ifdef DEBUG_PRINT
    fmt::print("***** {} end *****\n", __func__);
#endif
    return ret;
  }

  uint8_t *serialize_batch() { return buffer.get(); }

  void empty_buff() {
    // fmt::print("{}\n", __func__);
    cur_idx = 0;
  }

  static std::unique_ptr<uint8_t[]> deserialize(uint8_t *buf, size_t buf_sz) {
    size_t alloc_sz =
        (HMAC_N::signature_size * sizeof(uint8_t) + sizeof(uint64_t) +
         (batch_count * message_size) * sizeof(uint8_t)) /
        sizeof(uint8_t);
#ifdef DEBUG_PRINT
    if ((buf_sz != message_size * batch_count) && (buf_sz != alloc_sz))
      fmt::print(
          "[{}] WARNING: buf_sz ({}) != batch_count*message_size ({}) and != "
          "alloc_sz ({})\n",
          __PRETTY_FUNCTION__, buf_sz, (message_size * batch_count), alloc_sz);
#endif
    auto data = std::make_unique<uint8_t[]>(buf_sz);
    ::memcpy(data.get(), buf, buf_sz);
    return std::move(data);
  }

  static void print_batched(std::unique_ptr<uint8_t[]> msgs, size_t sz) {
    int min_idx = -1, max_idx = -1;
    for (auto i = 0ULL; i < batch_count; i++) {
      auto msg_data = std::make_unique<msg>();
      ::memcpy(reinterpret_cast<uint8_t *>(msg_data.get()),
               msgs.get() + i * sizeof(msg), sizeof(msg::header));
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
    for (auto i = 0ULL; i < batch_count; i++) {
      auto msg_data = std::make_unique<msg>();
      ::memcpy(reinterpret_cast<uint8_t *>(msg_data.get()),
               msgs.get() + i * sizeof(msg), sizeof(msg::header));
      if (i == 0) {
        min_idx = msg_data->hdr.seq_idx;
        prev_idx =
            min_idx - 1; // I will increase it later on for this iteration
      } else if (i == (batch_count - 1))
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
};
