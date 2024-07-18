#ifndef BALATROGETHER_ENCRYPT_H
#define BALATROGETHER_ENCRYPT_H

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <cstring>
#include <iostream>

struct RSAKeypair {
  std::string publicKey;
  std::string privateKey;
};

std::string encryptAES(std::string encoded_key, std::string encoded_iv, std::string data);
std::string decryptAES(std::string encoded_key, std::string encoded_iv, std::string data);
std::string encryptRSA(std::string publicKey, std::string data);
std::string decryptRSA(std::string privateKey, std::string data_base64);
std::string generateAESKey();
std::string generateAESIV();
RSAKeypair generateRSA();

#endif