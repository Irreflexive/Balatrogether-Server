#ifndef BALATROGETHER_SSL_UTIL_H
#define BALATROGETHER_SSL_UTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace balatrogether::ssl {
  SSL_CTX *create_context();
  void configure_context(SSL_CTX *ctx, bool debug);
}

#endif