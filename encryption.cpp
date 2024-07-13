#include "encryption.hpp"
#include <openssl/aes.h>
#include <string.h>

Encryption::Encryption()
{
  int ret = 0;
  RSA *r = NULL;
  BIGNUM *bne = NULL;
  BIO *bp_public = NULL, *bp_private = NULL;

  int bits = 2048;
  unsigned long e = RSA_F4;

  bne = BN_new();
  ret = BN_set_word(bne,e);
  if(ret != 1){
    goto free_all;
  }

  r = RSA_new();
  ret = RSA_generate_key_ex(r, bits, bne, NULL);
  if(ret != 1){
    goto free_all;
  }

  bp_public = BIO_new(BIO_s_mem());
  ret = PEM_write_bio_RSAPublicKey(bp_public, r);
  if(ret != 1){
    goto free_all;
  }

  bp_private = BIO_new(BIO_s_mem());
  ret = PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL);

  if(ret != 1){
free_all:
    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);
    throw "Failed to generate RSA key pair";
  } else {
    this->publicKey = bp_public;
    this->privateKey = bp_private;
    RSA_free(r);
    BN_free(bne);
  }
}

Encryption::~Encryption()
{
  BIO_free_all(this->publicKey);
  BIO_free_all(this->privateKey);
}

std::string Encryption::getPublicKey()
{
  RSA* r = PEM_read_bio_RSAPublicKey(this->publicKey, NULL, NULL, NULL);
  int rsa_len = RSA_size(r);
  char *publicKey = (char *)malloc(rsa_len);
  int ret = i2d_RSAPublicKey(r, (unsigned char **)&publicKey);
  if(ret == -1){
    RSA_free(r);
    free(publicKey);
    throw "Failed to get public key";
  }
  RSA_free(r);
  std::string publicKeyStr = publicKey;
  free(publicKey);
  return publicKeyStr;
}

std::string Encryption::decryptSymmetricKey(std::string encryptedKey)
{
  RSA *private_key = PEM_read_bio_RSAPrivateKey(this->privateKey, NULL, NULL, NULL);
  int rsa_len = RSA_size(private_key);
  char *decrypted = (char *)malloc(rsa_len);
  int ret = RSA_private_decrypt(rsa_len, (unsigned char *)encryptedKey.c_str(), (unsigned char *)decrypted, private_key, RSA_PKCS1_PADDING);
  if(ret == -1){
    RSA_free(private_key);
    free(decrypted);
    throw "Failed to decrypt message";
  }
  RSA_free(private_key);
  std::string decryptedKey = decrypted;
  free(decrypted);
  return decryptedKey;
}

std::string Encryption::encryptMessage(std::string plaintext, std::string symmetricKey)
{
  AES_KEY key;
  AES_set_encrypt_key((unsigned char *)symmetricKey.c_str(), 256, &key);
  int len = plaintext.size();
  char *ciphertext = (char *)malloc(len);
  unsigned char iv[AES_BLOCK_SIZE];
  memset(iv, 0x00, AES_BLOCK_SIZE);
  AES_cbc_encrypt((unsigned char *)plaintext.c_str(), (unsigned char *)ciphertext, len, &key, iv, AES_ENCRYPT);
  std::string ciphertextStr = ciphertext;
  free(ciphertext);
  return ciphertext;
}

std::string Encryption::decryptMessage(std::string ciphertext, std::string symmetricKey)
{
  AES_KEY key;
  AES_set_decrypt_key((unsigned char *)symmetricKey.c_str(), 256, &key);
  int len = ciphertext.size();
  char *decrypted = (char *)malloc(len);
  unsigned char iv[AES_BLOCK_SIZE];
  memset(iv, 0x00, AES_BLOCK_SIZE);
  AES_cbc_encrypt((unsigned char *)ciphertext.c_str(), (unsigned char *)decrypted, len, &key, iv, AES_DECRYPT);
  std::string decryptedStr = decrypted;
  free(decrypted);
  return decryptedStr;
}
