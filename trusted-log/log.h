#pragma once
#include "hmac_lib.h"
#include <fmt/printf.h>

template <size_t CTX_SIZE> class trusted_log {
public:
  struct log_entry {
    static constexpr size_t CtxSize = CTX_SIZE;
    static constexpr size_t HashSize = _hmac_size;
    static constexpr size_t AuthSize = _hmac_size;
    uint32_t sequencer;
    char context[CtxSize];
    char c_digest[HashSize]; // digest over the current entry + the previous
                             // entry
    char authenticator[AuthSize]; // authenticator size of the entry
    log_entry() {}
  };
  void print_entry_at(size_t idx, char *&ctx_ptr, size_t &ctx_sz) {
    auto entry_ptr = get_entry_at(idx);
    //   fmt::print("idx={}\t", idx);
    uint32_t sequencer;
    ::memcpy(&sequencer, entry_ptr, sizeof(sequencer));
    // fmt::print("sequencer={}\n", sequencer);
    ctx_ptr = entry_ptr + sizeof(sequencer);
    ctx_sz = log_entry::CtxSize;
#if 0
    fmt::print("[{}] c_digest=", __func__);
    for (auto i = 0ULL; i < log_entry::AuthSize; i++) {
      fmt::print("{}", reinterpret_cast<unsigned char *>(
                           (ctx_ptr + log_entry::CtxSize))[i]);
    }
    fmt::print("\n");
#endif
  }

  trusted_log() = delete;
  trusted_log(const trusted_log &other) = delete;
  trusted_log &operator=(const trusted_log &other) = delete;

  trusted_log &operator=(trusted_log &&other) = delete;

  trusted_log(trusted_log &&other) : trusted_log(other.log_size) {
    this->mem_log = std::move(other.mem_log);
    this->cur_idx = other.cur_idx;
    this->tail = other.tail;
    fmt::print("[{}] cur_idx={}, nb_max_entries={}, mem_log={}\n",
               __PRETTY_FUNCTION__, cur_idx, nb_max_entries,
               reinterpret_cast<uintptr_t>(mem_log.get()));
  }

  trusted_log(size_t log_sz)
      : nb_max_entries(log_sz / sizeof(log_entry)), cur_idx(0),
        log_size(log_sz), tail(nullptr) {
    mem_log = std::make_unique<char[]>(nb_max_entries * sizeof(log_entry));
    tail = mem_log.get();
    fmt::print("[{}] cur_idx={}, nb_max_entries={}, mem_log={}\n",
               __PRETTY_FUNCTION__, cur_idx, nb_max_entries,
               reinterpret_cast<uintptr_t>(mem_log.get()));
  }

  bool append(const log_entry &entry) {
    if (cur_idx >= nb_max_entries) {
      fmt::print("[{}] log is full\n", __PRETTY_FUNCTION__);
      return false;
    }
    append_entry(get_cur_log_idx(), entry);
    char *ctx_ptr = nullptr;
    size_t ctx_sz = 0;

    // print_entry_at(cur_idx, ctx_ptr, ctx_sz);
    inc_idx();
    return true;
  }

  char *get_tail() { return tail; }

  char *get_entry_at(uint32_t idx) { return get_log_entry_at_idx(idx); }

  uint32_t get_log_size() { return cur_idx; }

  void serialize_entry(char *pre_allocated_buf, uint32_t idx) {
    auto cur_entry = get_entry_at(idx);
    size_t offset = 0;
    ::memcpy(pre_allocated_buf + offset, cur_entry + offset,
             sizeof(log_entry::sequencer));
    offset += sizeof(log_entry::sequencer);
    ::memcpy(pre_allocated_buf + offset, cur_entry + offset,
             log_entry::CtxSize);
    offset += log_entry::CtxSize;
    ::memcpy(pre_allocated_buf + offset, cur_entry + offset,
             log_entry::HashSize);
    offset += log_entry::HashSize;
    ::memcpy(pre_allocated_buf + offset, cur_entry + offset,
             log_entry::AuthSize);
  }

  void serialize_tail(char *pre_allocated_buf) {
    size_t offset = 0;
    ::memcpy(pre_allocated_buf + offset, tail + offset,
             sizeof(log_entry::sequencer));
    offset += sizeof(log_entry::sequencer);
    ::memcpy(pre_allocated_buf + offset, tail + offset, log_entry::CtxSize);
    offset += log_entry::CtxSize;
    ::memcpy(pre_allocated_buf + offset, tail + offset, log_entry::HashSize);
    offset += log_entry::HashSize;
    ::memcpy(pre_allocated_buf + offset, tail + offset, log_entry::AuthSize);
  }

private:
  std::unique_ptr<char[]> mem_log;
  uint32_t nb_max_entries, cur_idx, log_size;
  char *tail;

  char *get_cur_log_idx() {
    return mem_log.get() + cur_idx * sizeof(log_entry);
  }

  char *get_log_entry_at_idx(size_t idx) {
    return mem_log.get() + idx * sizeof(log_entry);
  }

  void inc_idx() { cur_idx++; }

  void append_entry(char *pos, const log_entry &entry) {
    tail = pos;
    size_t offset = 0;
    ::memcpy(pos + offset, &entry.sequencer, sizeof(log_entry::sequencer));
    offset += sizeof(log_entry::sequencer);
    ::memcpy(pos + offset, entry.context, log_entry::CtxSize);
    offset += log_entry::CtxSize;
    ::memcpy(pos + offset, entry.c_digest, log_entry::HashSize);
#if 0
    fmt::print("[{}] pos+offset=", __func__);
    for (auto i = 0ULL; i < log_entry::HashSize; i++) {
      fmt::print("{}", reinterpret_cast<unsigned char *>(pos + offset)[i]);
    }
    fmt::print("\n");
    fmt::print("[{}] c_digest=", __func__);
    for (auto i = 0ULL; i < log_entry::HashSize; i++) {
      fmt::print("{}",
                 reinterpret_cast<const unsigned char *>(entry.c_digest)[i]);
    }
    fmt::print("\n");
#endif
    offset += log_entry::HashSize;
    ::memcpy(pos + offset, entry.authenticator, log_entry::AuthSize);
  }
};
