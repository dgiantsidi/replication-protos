#pragma once
#include "tnic_mock_api/api.h"
#include <fmt/printf.h>

std::string version = "";

template <size_t CTX_SIZE> class trusted_log {
public:
  struct log_entry {
    static constexpr size_t CtxSize = CTX_SIZE;
    static constexpr size_t HashSize = _hmac_size;
    uint32_t sequencer;
    char context[CtxSize];   // context = value
    char c_digest[HashSize]; // digest over the current entry + the previous
                             // entry c_digest(i) =
                             // h(sequencer||context||c_digest(i-1))
    log_entry() {}
    log_entry(const log_entry &other) {
      sequencer = other.sequencer;
      ::memcpy(context, other.context, CtxSize);
      ::memcpy(c_digest, other.c_digest, HashSize);
    }

    log_entry(log_entry &&other) {
      sequencer = other.sequencer;
      ::memcpy(context, other.context, CtxSize);
      ::memcpy(c_digest, other.c_digest, HashSize);
    }

    log_entry operator=(const log_entry &other) {
      sequencer = other.sequencer;
      ::memcpy(context, other.context, CtxSize);
      ::memcpy(c_digest, other.c_digest, HashSize);
      return *(this);
    }
    log_entry operator=(log_entry &&other) {
      sequencer = other.sequencer;
      ::memcpy(context, other.context, CtxSize);
      ::memcpy(c_digest, other.c_digest, HashSize);
      return *(this);
    }
  };

  bool a2m_append(const std::vector<char> &value) {
    if (cur_idx >= nb_max_entries) {
      return false;
    }
    log_entry to_be_added;
    to_be_added.sequencer = get_log_size();
    size_t len = value.size();
    if (len > log_entry::CtxSize) {
      fmt::print("[{}] trimming the value len as it is too big {}\n",
                 __PRETTY_FUNCTION__, len);
      len = log_entry::CtxSize;
    }
    ::memcpy(to_be_added.context, value.data(), len);
    // calculating the digest (chain)
    size_t tmp_sz = log_entry::HashSize + len + sizeof(log_entry::sequencer);
    std::unique_ptr<uint8_t[]> tmp = std::make_unique<uint8_t[]>(tmp_sz);
    size_t offset = 0;
    ::memcpy(tmp.get() + offset, &(to_be_added.sequencer),
             sizeof(log_entry::sequencer));
    offset += sizeof(log_entry::sequencer);
    ::memcpy(tmp.get() + offset, to_be_added.context, len);
    offset += log_entry::CtxSize;

    ::memcpy(tmp.get() + offset, get_tail_digest(), log_entry::HashSize);

    // auto [c_digest, d_len] = hmac_sha256(tmp.get(), tmp_sz);
#ifdef FPGA
#warning "FPGA-version is evaluated"
    auto [c_digest, d_len] = tnic_api::FPGA_get_attestation(tmp.get(), tmp_sz);
#elif AMD
#warning "AMD-version is evaluated"
    auto [c_digest, d_len] =
        tnic_api::AMDSEV_get_attestation(tmp.get(), tmp_sz);
#else
#warning "Native or SGX-based versions are evaluated"
    auto [c_digest, d_len] =
        tnic_api::native_get_attestation(tmp.get(), tmp_sz);
#endif
    if (d_len != log_entry::HashSize) {
      fmt::print("[{}] Error #1 d_len != log_entry::HashSize\n",
                 __PRETTY_FUNCTION__);
    }
    ::memcpy(to_be_added.c_digest, c_digest.data(), d_len);
    return append(to_be_added);
  }

  void print_entry_at(size_t idx, char *&ctx_ptr, size_t &ctx_sz) {
#ifdef DEBUG_PRINT
    auto entry_ptr = get_entry_at(idx);
    fmt::print("idx={}\t", idx);
    uint32_t sequencer;
    ::memcpy(&sequencer, entry_ptr, sizeof(sequencer));
    fmt::print("sequencer={}\n", sequencer);
    ctx_ptr = entry_ptr + sizeof(sequencer);
    ctx_sz = log_entry::CtxSize;
    fmt::print("[{}] c_digest=", __func__);
    for (auto i = 0ULL; i < log_entry::HashSize; i++) {
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
#ifdef FPGA
#warning "FPGA-version is evaluated"
    version = "FPGA";
#elif AMD
    version = "AMD";
#else
    version = "SGX";
#endif
    this->mem_log = std::move(other.mem_log);
    this->cur_idx = other.cur_idx;
    this->tail = other.tail;
    fmt::print("[{} w/ version={}] cur_idx={}, nb_max_entries={}, mem_log={}\n",
               __PRETTY_FUNCTION__, version, cur_idx, nb_max_entries,
               reinterpret_cast<uintptr_t>(mem_log.get()));
  }

  trusted_log(size_t log_sz)
      : nb_max_entries(log_sz / sizeof(log_entry)), cur_idx(0),
        log_size(log_sz), tail(nullptr) {
#ifdef FPGA
#warning "FPGA-version is evaluated"
    version = "FPGA";
#elif AMD
    version = "AMD";
#else
    version = "SGX";
#endif
    mem_log = std::make_unique<char[]>(nb_max_entries * sizeof(log_entry));
    tail = mem_log.get();
    fmt::print("[{} w/ version={}] cur_idx={}, nb_max_entries={}, mem_log={}\n",
               __PRETTY_FUNCTION__, version, cur_idx, nb_max_entries,
               reinterpret_cast<uintptr_t>(mem_log.get()));
  }

  struct lookup_attestation {
    enum { UNASSIGNED, FORGOTTEN, REGULAR };
    uint32_t w;
    struct log_entry e;
    lookup_attestation(int mode) { w = mode; };
  };

  lookup_attestation a2m_lookup(size_t idx, uint64_t nonce) {
    // TODO: use nonce correctly
    if (idx > get_log_size()) {
      return lookup_attestation(lookup_attestation::UNASSIGNED);
    }
    if (idx < 0) {
      return lookup_attestation(lookup_attestation::FORGOTTEN);
    }

    lookup_attestation ret_a(lookup_attestation::REGULAR);
    auto entry_ptr = get_entry_at(idx);
#ifdef DEBUG_PRINT
    fmt::print("idx={}\t", idx);
#endif
    ::memcpy(&(ret_a.e.sequencer), entry_ptr, sizeof(log_entry::sequencer));
#ifdef DEBUG_PRINT
    fmt::print("sequencer={}\n", ret_a.sequencer);
#endif
    auto *ctx_ptr = entry_ptr + sizeof(log_entry::sequencer);
    ::memcpy(ret_a.e.context, ctx_ptr, sizeof(log_entry::CtxSize));
    ctx_ptr += log_entry::CtxSize;
    ::memcpy(ret_a.e.c_digest, ctx_ptr, sizeof(log_entry::HashSize));
    return ret_a;
  }

  bool append(const log_entry &entry) {
    if (cur_idx >= nb_max_entries) {
      fmt::print("[{}] log is full\n", __PRETTY_FUNCTION__);
      return false;
    }
    append_entry(get_cur_log_idx(), entry);
    char *ctx_ptr = nullptr;
    size_t ctx_sz = 0;

    print_entry_at(cur_idx, ctx_ptr, ctx_sz);
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
  }

  void serialize_tail(char *pre_allocated_buf) {
    size_t offset = 0;
    ::memcpy(pre_allocated_buf + offset, tail + offset,
             sizeof(log_entry::sequencer));
    offset += sizeof(log_entry::sequencer);
    ::memcpy(pre_allocated_buf + offset, tail + offset, log_entry::CtxSize);
    offset += log_entry::CtxSize;
    ::memcpy(pre_allocated_buf + offset, tail + offset, log_entry::HashSize);
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

  uint8_t *get_tail_digest() {
    return reinterpret_cast<uint8_t *>(
        get_cur_log_idx() + sizeof(log_entry::sequencer) + log_entry::CtxSize);
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
#ifdef DEBUG_PRINT
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
  }
};
