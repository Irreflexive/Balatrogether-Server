#include "encrypt.hpp"

std::string base64_encode(std::string in)
{
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO* bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, in.c_str(), in.size());
  BIO_flush(b64);

  char* buffer;
  long length = BIO_get_mem_data(bmem, &buffer);
  std::string result(buffer, length);

  BIO_free_all(b64);
  return result;
}

std::string base64_decode(std::string in)
{
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO* bmem = BIO_new_mem_buf((void*)in.c_str(), in.size());
  bmem = BIO_push(b64, bmem);

  char* buffer = new char[in.size()];
  BIO_read(bmem, buffer, in.size());

  BIO_free_all(bmem);
  std::string result(buffer, in.size());
  delete[] buffer;
  return result;
}

std::string encryptAES(std::string encoded_key, std::string encoded_iv, std::string data) {
  std::string key = base64_decode(encoded_key);
  std::string iv = base64_decode(encoded_iv);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    throw "Failed to create EVP_CIPHER_CTX";
  }

  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str())) {
    throw "Failed to initialize encryption";
  }

  unsigned char* encryptedData = new unsigned char[data.size() + EVP_MAX_BLOCK_LENGTH];
  int len;
  int ciphertext_len;

  if (1 != EVP_EncryptUpdate(ctx, encryptedData, &len, (const unsigned char*)data.c_str(), data.size())) {
    throw "Failed to encrypt data";
  }
  ciphertext_len = len;

  if (1 != EVP_EncryptFinal_ex(ctx, encryptedData + len, &len)) {
    throw "Failed to finalize encryption";
  }
  ciphertext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  std::string result((char*)encryptedData, ciphertext_len);
  delete[] encryptedData;
  return result;
}

std::string decryptAES(std::string encoded_key, std::string encoded_iv, std::string data) {
  std::string key = base64_decode(encoded_key);
  std::string iv = base64_decode(encoded_iv);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    throw "Failed to create EVP_CIPHER_CTX";
  }

  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (const unsigned char*)key.c_str(), (const unsigned char*)iv.c_str())) {
    throw "Failed to initialize decryption";
  }

  unsigned char* decryptedData = new unsigned char[data.size() + EVP_MAX_BLOCK_LENGTH];
  int len;
  int plaintext_len;

  if (1 != EVP_DecryptUpdate(ctx, decryptedData, &len, (const unsigned char*)data.c_str(), data.size())) {
    throw "Failed to decrypt data";
  }
  plaintext_len = len;

  if (1 != EVP_DecryptFinal_ex(ctx, decryptedData + len, &len)) {
    throw "Failed to finalize decryption";
  }
  plaintext_len += len;

  EVP_CIPHER_CTX_free(ctx);

  std::string result((char*)decryptedData, plaintext_len);
  delete[] decryptedData;
  return result;
}

std::string encryptRSA(std::string publicKey, std::string data)
{
  int plaintext_len = data.size();
  data.insert(0, std::to_string(plaintext_len) + "\n");
  RSA* rsa = RSA_new();
  BIO* keybio = BIO_new_mem_buf((void*)publicKey.c_str(), publicKey.size());
  if (!PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL)) {
    BIO_free_all(keybio);
    RSA_free(rsa);
    throw "Failed to read public key from BIO";
  }

  int rsa_len = RSA_size(rsa);
  std::string result;
  for (size_t i = 0; i < data.size(); i += rsa_len - 42) {
    unsigned char* encryptedData = new unsigned char[rsa_len];
    if (-1 == RSA_public_encrypt(rsa_len - 42, (const unsigned char*)data.substr(i, rsa_len - 42).c_str(), encryptedData, rsa, RSA_PKCS1_OAEP_PADDING)) {
      BIO_free_all(keybio);
      RSA_free(rsa);
      throw "Failed to encrypt data";
    }

    result += base64_encode(std::string((char*)encryptedData, rsa_len));
    delete[] encryptedData;
  }

  return result;
}

std::string decryptRSA(std::string privateKey, std::string data_base64)
{
  std::string data = base64_decode(data_base64);
  RSA* rsa = RSA_new();
  BIO* keybio = BIO_new_mem_buf((void*)privateKey.c_str(), privateKey.size());
  if (!PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL)) {
    BIO_free_all(keybio);
    RSA_free(rsa);
    throw "Failed to read private key from BIO";
  }

  int rsa_len = RSA_size(rsa);
  std::string result;
  unsigned char* decryptedData = new unsigned char[rsa_len - 42];
  int dec_len = RSA_private_decrypt(rsa_len, (const unsigned char*)data.substr(0, rsa_len).c_str(), decryptedData, rsa, RSA_PKCS1_OAEP_PADDING);
  if (-1 == dec_len) {
    BIO_free_all(keybio);
    RSA_free(rsa);
    delete[] decryptedData;
    throw "Failed to decrypt data";
  }

  std::string decryptedBlock((char*)decryptedData, dec_len);

  int decrypted_len;
  int length_size;
  for (int i = 0; i < dec_len; i++) {
    if (decryptedBlock[i] == '\n') {
      length_size = i;
      char temp[i];
      memset(temp, 0, i);
      strncpy(temp, decryptedBlock.c_str(), i);
      decrypted_len = atoi(temp);
      break;
    }
  }

  if (dec_len >= decrypted_len || data.size() <= rsa_len) {
    result = decryptedBlock.substr(length_size + 1, decrypted_len);
  } else {
    BIO_free_all(keybio);
    RSA_free(rsa);
    delete[] decryptedData;
    throw "Unexpected data length";
  }
  
  delete[] decryptedData;
  return result;
}

std::string generateAESKey()
{
  // Generate random key
  unsigned char key[EVP_MAX_KEY_LENGTH];
  if (1 != RAND_bytes(key, EVP_MAX_KEY_LENGTH)) {
    throw "Failed to generate AES key";
  }

  return base64_encode(std::string((char*)key, EVP_MAX_KEY_LENGTH));
}

std::string generateAESIV()
{
  // Generate random IV
  unsigned char iv[EVP_MAX_IV_LENGTH];
  if (1 != RAND_bytes(iv, EVP_MAX_IV_LENGTH)) {
    throw "Failed to generate AES IV";
  }

  return base64_encode(std::string((char*)iv, EVP_MAX_IV_LENGTH));
}

RSAKeypair generateRSA()
{
  RSAKeypair keypair;

  // Generate RSA keypair
  RSA* rsa = RSA_new();
  BIGNUM* bn = BN_new();
  BN_set_word(bn, RSA_F4);
  RSA_generate_key_ex(rsa, 2048, bn, NULL);

  // Get public key
  BIO* keybio = BIO_new(BIO_s_mem());
  if (!PEM_write_bio_RSA_PUBKEY(keybio, rsa)) {
    BIO_free_all(keybio);
    RSA_free(rsa);
    BN_free(bn);
    throw "Failed to write public key to BIO";
  }
  char* publicKey;
  long length = BIO_get_mem_data(keybio, &publicKey);
  keypair.publicKey = std::string(publicKey, length);
  BIO_free_all(keybio);

  // Get private key
  keybio = BIO_new(BIO_s_mem());
  if (!PEM_write_bio_RSAPrivateKey(keybio, rsa, NULL, NULL, 0, NULL, NULL)) {
    BIO_free_all(keybio);
    RSA_free(rsa);
    BN_free(bn);
    throw "Failed to write private key to BIO";
  }
  char* privateKey;
  length = BIO_get_mem_data(keybio, &privateKey);
  keypair.privateKey = std::string(privateKey, length);
  BIO_free_all(keybio);

  RSA_free(rsa);
  BN_free(bn);

  return keypair;
}