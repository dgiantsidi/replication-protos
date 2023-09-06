#include <cstring>
#include <memory>
#include <openssl/evp.h>
#include <stdio.h>

constexpr int max_hash_sz = EVP_MAX_MD_SIZE; // that is 64B

std::unique_ptr<uint8_t[]> get_sha256(const char *data, size_t data_sz) {
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  const EVP_MD *md = EVP_sha256();

  std::unique_ptr<uint8_t[]> md_value =
      std::make_unique<uint8_t[]>(EVP_MAX_MD_SIZE);
  uint32_t md_len;

  EVP_MD_CTX_init(mdctx);
  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, data, data_sz);
  EVP_DigestFinal_ex(mdctx, md_value.get(), &md_len);
  EVP_MD_CTX_free(mdctx);
  // do not move as you prevent copy elision;
  // return std::move(md_value);
  return std::move(md_value);
}
