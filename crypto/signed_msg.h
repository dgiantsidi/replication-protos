#pragma once
#include <fmt/printf.h>
//#include "crypto/rsa_lib.h"
#include "tnic_mock_api/api.h"

//#include "crypto/hmac_lib.h"
#include <memory>
#if 0
namespace RSA_N {
	inline int signature_size = 256; // RSA_size(rsa);

	inline bool sign_msg(uint8_t *plain_text, size_t plain_text_sz, uint8_t *private_key,
			uint8_t *signed_msg_buf) {

		uint8_t *signature = new uint8_t[signature_size];
		const char *data = reinterpret_cast<char *>(plain_text);
		int encrypted_length = priv_sign_sha256(data, plain_text_sz, private_key,
				signature, signature_size);

		if (encrypted_length == -1) {
			fmt::print("[{}] error (encrypted_lenght={})\n", __func__,
					encrypted_length);
			return false;
		}
		if (encrypted_length != signature_size) {
			fmt::print("[{}] encrypted_lenght={} != signature_size={}\n", __func__,
					encrypted_length, signature_size);
			return false;
		}
		size_t offset = 0;
		::memcpy(signed_msg_buf, signature, signature_size);
		offset += signature_size;
		::memcpy(signed_msg_buf + offset, &plain_text_sz, sizeof(uint64_t));
		offset += sizeof(uint64_t);
		::memcpy(signed_msg_buf + offset, plain_text, plain_text_sz);

		delete[] signature;
		return true;
	}

	inline bool verify_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
		unsigned char decrypted_hash[max_hash_sz];
		uint8_t signature[signature_size];
		::memcpy(signature, signed_msg_buff, signature_size);
		int decrypted_length =
			pub_verify_sha256(signature, signature_size, public_key, decrypted_hash);

		uint64_t msg_size_;
		::memcpy(&msg_size_, signed_msg_buff + signature_size, sizeof(uint64_t));

		const char *payload = reinterpret_cast<char *>(signed_msg_buff) +
			(signature_size + sizeof(uint64_t));

		unsigned char dec_hash[max_hash_sz];
		::memcpy(dec_hash, decrypted_hash, max_hash_sz);
		auto calc_hash = get_sha256(payload, msg_size_);
		// FIXME
		if (::memcmp(calc_hash.get(), dec_hash, max_hash_sz) == 0) {
			// fmt::print("[{}] correct\n", __func__);
			return true;
		}
		fmt::print("[{}] validation failed .. computed_hash != dec_hash\n", __func__);

		return false;
	}

	inline std::tuple<int, std::unique_ptr<uint8_t[]>>
		verify_get_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
			unsigned char decrypted_hash[max_hash_sz];
			uint8_t signature[signature_size];
			::memcpy(signature, signed_msg_buff, signature_size);
			int decrypted_length =
				pub_verify_sha256(signature, signature_size, public_key, decrypted_hash);

			uint64_t msg_size_;
			::memcpy(&msg_size_, signed_msg_buff + signature_size, sizeof(uint64_t));
			const char *payload = reinterpret_cast<char *>(signed_msg_buff) +
				(signature_size + sizeof(uint64_t));

			unsigned char dec_hash[max_hash_sz];
			::memcpy(dec_hash, decrypted_hash, max_hash_sz);

			auto data_payload = std::make_unique<uint8_t[]>(msg_size_);
			auto calc_hash = get_sha256(payload, msg_size_);

			// FIXME
			if (::memcmp(calc_hash.get(), dec_hash, max_hash_sz) == 0) {
				::memcpy(data_payload.get(),
						(signed_msg_buff + (signature_size + sizeof(uint64_t))),
						msg_size_);
				return std::make_tuple(msg_size_, std::move(data_payload));
			}
			fmt::print("[{}] validation failed .. computed_hash != dec_hash\n", __func__);
			return std::make_tuple(-1, std::move(data_payload));
		}
} // end namespace RSA
#endif
namespace HMAC_N {
inline int _max_hash_sz = EVP_MAX_MD_SIZE / 2; // EVP_sha256;
int signature_size = _max_hash_sz;

inline bool sign_msg(uint8_t *plain_text, size_t plain_text_sz,
                     uint8_t *private_key /* unused */,
                     uint8_t *signed_msg_buf) {

#ifdef FPGA
#warning "FPGA-version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::FPGA_get_attestation(plain_text, plain_text_sz);
#elif AMD
#warning "AMD-version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::AMDSEV_get_attestation(plain_text, plain_text_sz);
#elif SGX
#warning "SGX-based versions is evaluated"
  auto [signed_digest, sz] =
      tnic_api::SGX_get_attestation(plain_text, plain_text_sz);
#else
#warning "Native version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::native_get_attestation(plain_text, plain_text_sz);
#endif

  //		auto [signed_digest, sz] = hmac_sha256(plain_text,
  // plain_text_sz);

  if (sz <= 0) {
    fmt::print("[{}] error (sz={})\n", __func__, sz);
    return false;
  }
  if (sz != _max_hash_sz) {
    fmt::print("[{}] error (sz={})\n", __func__, sz);
    return false;
  }

  size_t offset = 0;
  ::memcpy(signed_msg_buf, signed_digest.data(), sz);
  offset += sz;
  ::memcpy(signed_msg_buf + offset, &plain_text_sz, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  ::memcpy(signed_msg_buf + offset, plain_text, plain_text_sz);
  return true;
}

inline bool verify_msg(uint8_t *signed_msg_buff,
                       uint8_t *public_key /* unused */) {
  unsigned char calculated_hash[_max_hash_sz];
  uint64_t plain_text_sz = -1;
  ::memcpy(&plain_text_sz, signed_msg_buff + _max_hash_sz, sizeof(uint64_t));
  if (plain_text_sz == -1) {
    fmt::print("[{}] error\n", __PRETTY_FUNCTION__);
  }
  auto [signed_digest, sz] = hmac_sha256(
      signed_msg_buff + _max_hash_sz + sizeof(plain_text_sz), plain_text_sz);
  auto calc_hash = reinterpret_cast<char *>(signed_digest.data());

  auto recv_hash = reinterpret_cast<char *>(signed_msg_buff);
  // FIXME
  if (::memcmp(calc_hash, recv_hash, _max_hash_sz) == 0) {
    // fmt::print("[{}] correct\n", __func__);
    return true;
  }
  fmt::print("[{}] validation failed .. computed_hash != dec_hash\n", __func__);

  return true;
}

std::tuple<int, std::unique_ptr<uint8_t[]>>
verify_get_msg(uint8_t *signed_msg_buff, uint8_t *public_key) {
  // TODO: implement me!
  unsigned char calculated_hash[_max_hash_sz];
  uint64_t plain_text_sz = -1;
  ::memcpy(&plain_text_sz, signed_msg_buff + _max_hash_sz, sizeof(uint64_t));
  if (plain_text_sz == -1) {
    fmt::print("[{}] error\n", __PRETTY_FUNCTION__);
  }
  auto *plain_text = signed_msg_buff + _max_hash_sz + sizeof(plain_text_sz);
#ifdef FPGA
#warning "FPGA-version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::FPGA_get_attestation(plain_text, plain_text_sz);
#elif AMD
#warning "AMD-version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::AMDSEV_get_attestation(plain_text, plain_text_sz);
#elif SGX
#warning "SGX-based versions is evaluated"
  auto [signed_digest, sz] =
      tnic_api::SGX_get_attestation(plain_text, plain_text_sz);
#else
#warning "Native version is evaluated"
  auto [signed_digest, sz] =
      tnic_api::native_get_attestation(plain_text, plain_text_sz);
#endif
  //	auto [signed_digest, sz] = hmac_sha256(signed_msg_buff + _max_hash_sz +
  // sizeof(plain_text_sz), plain_text_sz);
  auto calc_hash = reinterpret_cast<char *>(signed_digest.data());

  auto recv_hash = reinterpret_cast<char *>(signed_msg_buff);
  // FIXME
  if (::memcmp(calc_hash, recv_hash, _max_hash_sz) == 0) {
    // fmt::print("[{}] correct\n", __func__);
    std::unique_ptr<uint8_t[]> data =
        std::make_unique<uint8_t[]>(plain_text_sz);
    ::memcpy(data.get(), signed_msg_buff + _max_hash_sz + sizeof(plain_text_sz),
             plain_text_sz);
    return {plain_text_sz, std::move(data)};
  }
  fmt::print("[{}] validation failed .. computed_hash != dec_hash\n", __func__);

  return {-1, std::make_unique<uint8_t[]>(1)};
}
} // namespace HMAC_N
