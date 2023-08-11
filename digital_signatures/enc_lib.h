#pragma once
#include <city.h>
#include <cstring>
#include <fmt/printf.h>
#include <iostream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

int padding = RSA_PKCS1_PADDING;

RSA *createRSA(unsigned char *key, int public_enc) {
  RSA *rsa = nullptr;
  BIO *keybio;
  keybio = BIO_new_mem_buf(key, -1);
  if (keybio == nullptr) {
    fmt::print("[{}] Failed to create key BIO.\n", __PRETTY_FUNCTION__);
    return 0;
  }

  if (public_enc != 0) {
    rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
  } else {
    rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
  }
  if (rsa == nullptr) {
    fmt::print("[{}] Failed to create RSA.\n", __PRETTY_FUNCTION__);
  }
  BIO_free(keybio);

  return rsa;
}

#if 0
int public_encrypt(unsigned char * data, int data_len, unsigned char * key, unsigned char *encrypted) {
	RSA * rsa = createRSA(key, 1);
	int result = RSA_public_encrypt(data_len,data,encrypted,rsa,padding);
	return result;
}

int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted) {
	RSA * rsa = createRSA(key,0);
	int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,padding);
	return result;
}
#endif

int private_encrypt(unsigned char *data, int data_len, unsigned char *key,
                    unsigned char *encrypted) {
  RSA *rsa = createRSA(key, 0);
  int result = RSA_private_encrypt(data_len, data, encrypted, rsa, padding);
  RSA_free(rsa);
  return result;
}

int public_decrypt(unsigned char *enc_data, int data_len, unsigned char *key,
                   unsigned char *decrypted) {
  RSA *rsa = createRSA(key, 1);
  int result = RSA_public_decrypt(data_len, enc_data, decrypted, rsa, padding);
  RSA_free(rsa);
  return result;
}

int priv_sign(const char *data, int data_len, uint8_t *key,
              uint8_t *&signed_msg, int signature_size) {
#ifdef PRINT_DEBUG
  fmt::print("[{}]\n", __func__);
#endif
  auto a = CityHash32(data, data_len);
#ifdef PRINT_DEBUG
  fmt::print("[{}] Text to be encrypted={}\n", __PRETTY_FUNCTION__, a);
#endif

  // signed_msg = new uint8_t[signature_size];
  int k = private_encrypt(reinterpret_cast<uint8_t *>(&a), sizeof(uint32_t),
                          key, reinterpret_cast<uint8_t *>(signed_msg));
#ifdef PRINT_DEBUG
  fmt::print("\n");
  for (auto i = 0; i < k; i++) {
    fmt::print("{}", signed_msg[i]);
  }
  fmt::print("\n");
#endif
  return k;
}

int pub_verify(unsigned char *enc_data, int data_len, unsigned char *key,
               unsigned char *decrypted) {
#ifdef PRINT_DEBUG2
  fmt::print("[{}] data_len={}\n", __PRETTY_FUNCTION__, data_len);
  fmt::print("\n");
#endif
#ifdef PRINT_DEBUG2
  for (auto i = 0; i < data_len; i++) {
    fmt::print("{}", enc_data[i]);
  }
  fmt::print("\n");
#endif
  auto i = public_decrypt(enc_data, data_len, key, decrypted);
  uint32_t a;
  ::memcpy(&a, decrypted, sizeof(uint32_t));
#ifdef PRINT_DEBUG2
  fmt::print("{}\n", a);
#endif
  return i;
}

void print_error(const char *msg) {
  auto err = std::make_unique<char[]>(130);
  ERR_load_crypto_strings();
  ERR_error_string(ERR_get_error(), err.get());
  fmt::print("{} ERROR: {}\n", msg, err.get());
}

char publicKey[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy8Dbv8prpJ/0kKhlGeJY\n"
    "ozo2t60EG8L0561g13R29LvMR5hyvGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+\n"
    "vw1HocOAZtWK0z3r26uA8kQYOKX9Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQAp\n"
    "fc9jB9nTzphOgM4JiEYvlV8FLhg9yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68\n"
    "i6T4nNq7NWC+UNVjQHxNQMQMzU6lWCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoV\n"
    "PpY72+eVthKzpMeyHkBn7ciumk5qgLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUy\n"
    "wQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

char privateKey[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "MIIEowIBAAKCAQEAy8Dbv8prpJ/0kKhlGeJYozo2t60EG8L0561g13R29LvMR5hy\n"
    "vGZlGJpmn65+A4xHXInJYiPuKzrKUnApeLZ+vw1HocOAZtWK0z3r26uA8kQYOKX9\n"
    "Qt/DbCdvsF9wF8gRK0ptx9M6R13NvBxvVQApfc9jB9nTzphOgM4JiEYvlV8FLhg9\n"
    "yZovMYd6Wwf3aoXK891VQxTr/kQYoq1Yp+68i6T4nNq7NWC+UNVjQHxNQMQMzU6l\n"
    "WCX8zyg3yH88OAQkUXIXKfQ+NkvYQ1cxaMoVPpY72+eVthKzpMeyHkBn7ciumk5q\n"
    "gLTEJAfWZpe4f4eFZj/Rc8Y8Jj2IS5kVPjUywQIDAQABAoIBADhg1u1Mv1hAAlX8\n"
    "omz1Gn2f4AAW2aos2cM5UDCNw1SYmj+9SRIkaxjRsE/C4o9sw1oxrg1/z6kajV0e\n"
    "N/t008FdlVKHXAIYWF93JMoVvIpMmT8jft6AN/y3NMpivgt2inmmEJZYNioFJKZG\n"
    "X+/vKYvsVISZm2fw8NfnKvAQK55yu+GRWBZGOeS9K+LbYvOwcrjKhHz66m4bedKd\n"
    "gVAix6NE5iwmjNXktSQlJMCjbtdNXg/xo1/G4kG2p/MO1HLcKfe1N5FgBiXj3Qjl\n"
    "vgvjJZkh1as2KTgaPOBqZaP03738VnYg23ISyvfT/teArVGtxrmFP7939EvJFKpF\n"
    "1wTxuDkCgYEA7t0DR37zt+dEJy+5vm7zSmN97VenwQJFWMiulkHGa0yU3lLasxxu\n"
    "m0oUtndIjenIvSx6t3Y+agK2F3EPbb0AZ5wZ1p1IXs4vktgeQwSSBdqcM8LZFDvZ\n"
    "uPboQnJoRdIkd62XnP5ekIEIBAfOp8v2wFpSfE7nNH2u4CpAXNSF9HsCgYEA2l8D\n"
    "JrDE5m9Kkn+J4l+AdGfeBL1igPF3DnuPoV67BpgiaAgI4h25UJzXiDKKoa706S0D\n"
    "4XB74zOLX11MaGPMIdhlG+SgeQfNoC5lE4ZWXNyESJH1SVgRGT9nBC2vtL6bxCVV\n"
    "WBkTeC5D6c/QXcai6yw6OYyNNdp0uznKURe1xvMCgYBVYYcEjWqMuAvyferFGV+5\n"
    "nWqr5gM+yJMFM2bEqupD/HHSLoeiMm2O8KIKvwSeRYzNohKTdZ7FwgZYxr8fGMoG\n"
    "PxQ1VK9DxCvZL4tRpVaU5Rmknud9hg9DQG6xIbgIDR+f79sb8QjYWmcFGc1SyWOA\n"
    "SkjlykZ2yt4xnqi3BfiD9QKBgGqLgRYXmXp1QoVIBRaWUi55nzHg1XbkWZqPXvz1\n"
    "I3uMLv1jLjJlHk3euKqTPmC05HoApKwSHeA0/gOBmg404xyAYJTDcCidTg6hlF96\n"
    "ZBja3xApZuxqM62F6dV4FQqzFX0WWhWp5n301N33r0qR6FumMKJzmVJ1TA8tmzEF\n"
    "yINRAoGBAJqioYs8rK6eXzA8ywYLjqTLu/yQSLBn/4ta36K8DyCoLNlNxSuox+A5\n"
    "w6z2vEfRVQDq4Hm4vBzjdi3QfYLNkTiTqLcvgWZ+eX44ogXtdTDO7c+GeMKWz4XX\n"
    "uJSUVL5+CVjKLjZEJ6Qc2WZLl94xSwL71E41H4YciVnSCQxVc4Jw\n"
    "-----END RSA PRIVATE KEY-----\n";
