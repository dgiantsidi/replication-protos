#include "enc_lib.h"
#include <cstring>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

int main() {

  // char plainText[2048/8] = "Hello this is Ravi"; //key length : 2048

  char plainText[1024];
  for (auto i = 0ULL; i < 1024; i++) {
    plainText[i] = 'd';
  }

  unsigned char *encrypted;
  unsigned char decrypted[4098] = {};

#if 0
	int encrypted_length= public_encrypt(reinterpret_cast<uint8_t*>(plainText), std::strlen(plainText), reinterpret_cast<uint8_t*>(publicKey), encrypted);
	if(encrypted_length == -1) {
		printLastError("Public Encrypt failed ");
		exit(0);
	}
	printf("Encrypted length =%d\n",encrypted_length);

	int decrypted_length = private_decrypt(encrypted, encrypted_length, reinterpret_cast<uint8_t*>(privateKey), decrypted);
	if(decrypted_length == -1)
	{
		printLastError("Private Decrypt failed ");
		exit(0);
	}
	printf("Decrypted Text =%s\n",decrypted);
	printf("Decrypted Length =%d\n",decrypted_length);

#endif
  int encrypted_length =
      priv_sign(plainText, std::strlen(plainText),
                reinterpret_cast<uint8_t *>(privateKey), encrypted);
  //	int encrypted_length=
  // private_encrypt(reinterpret_cast<uint8_t*>(plainText),
  // std::strlen(plainText), reinterpret_cast<uint8_t*>(privateKey), encrypted);
  if (encrypted_length == -1) {
    printLastError("Private Encrypt failed");
    exit(0);
  }
  printf("Encrypted length =%d\n", encrypted_length);

  // int decrypted_length = public_decrypt(encrypted, encrypted_length,
  // reinterpret_cast<uint8_t*>(publicKey), decrypted);
  int decrypted_length =
      pub_verify(encrypted, encrypted_length,
                 reinterpret_cast<uint8_t *>(publicKey), decrypted);
  if (decrypted_length == -1) {
    printLastError("Public Decrypt failed");
    exit(0);
  }
  uint32_t decrypted_hash;
  ::memcpy(&decrypted_hash, decrypted, sizeof(decrypted_hash));
  // printf("Decrypted Text =%s\n",decrypted);
  fmt::print("[{}] Decrypted text={}\n", __PRETTY_FUNCTION__, decrypted_hash);
  fmt::print("[{}] Decrypted length={}\n", __PRETTY_FUNCTION__,
             decrypted_length);
  // printf("Decrypted Length =%d\n",decrypted_length);
  return 0;
}
