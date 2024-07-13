#ifndef BALATROGETHER_ENCRYPTION_H
#define BALATROGETHER_ENCRYPTION_H

#include <stdio.h>
#include <string>
#include <openssl/rsa.h>
#include <openssl/pem.h>

class Encryption {
	public:
		Encryption();
		~Encryption();
		std::string getPublicKey();
		std::string decryptSymmetricKey(std::string encryptedKey);
		std::string encryptMessage(std::string plaintext, std::string symmetricKey);
		std::string decryptMessage(std::string ciphertext, std::string symmetricKey);
	private:
		BIO *publicKey;
		BIO *privateKey;
};

#endif