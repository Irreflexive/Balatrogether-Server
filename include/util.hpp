#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string>
#include "types.hpp"

using namespace balatrogether;

namespace balatrogether {
  string getpath();
  json success(string cmd, json data);
  json success(string cmd);
  json error(string msg);

  namespace logger {
    void setDebugOutputEnabled(bool enabled);

    int info(string format, ...);
    int debug(string format, ...);
    int error(string format, ...);
  }

  namespace ssl {
    SSL_CTX *create_context();
    void configure_context(SSL_CTX *ctx, bool debug);
  }
}

#endif