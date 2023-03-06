

struct msg {
  static constexpr size_t key_sz = 8;
  static constexpr size_t value_sz = 16;
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
