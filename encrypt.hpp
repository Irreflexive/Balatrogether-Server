#ifndef BALATROGETHER_ENCRYPT_H
#define BALATROGETHER_ENCRYPT_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);

#endif