#include <cstring>
#include <memory>
#include <openssl/evp.h>
#include <stdio.h>

static void print_digest(uint8_t *data, size_t md_len) {

  printf("Digest (%ldB) is: ", md_len);
  for (auto i = 0ULL; i < md_len; i++)
    printf("%02x", data[i]);
  printf("\n");
}

void test(const EVP_MD *algo) {
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  const EVP_MD *md = algo;

  std::unique_ptr<char[]> mess1 = std::make_unique<char[]>(sizeof(uint64_t));
  unsigned char md_value[EVP_MAX_MD_SIZE];
  uint32_t md_len;

#if 0
	OpenSSL_add_all_digests();
	if(!argv[1]) {
		printf("Usage: mdtest digestname\n");
		exit(1);
	}
#endif

  EVP_MD_CTX_init(mdctx);
  EVP_DigestInit_ex(mdctx, md, NULL);
  EVP_DigestUpdate(mdctx, mess1.get(), sizeof(uint64_t));
  //     EVP_DigestUpdate(mdctx, mess2, strlen(mess2));
  EVP_DigestFinal_ex(mdctx, md_value, &md_len);
  EVP_MD_CTX_free(mdctx);
  print_digest(md_value, md_len);
}

int main(int argc, char *argv[]) {

  test(EVP_sha512());
  test(EVP_sha256());

  return 0;
}
