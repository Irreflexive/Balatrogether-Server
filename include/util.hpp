#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

std::string getpath();
json success(std::string cmd, json data);
json success(std::string cmd);
json error(std::string msg);

namespace logger {
  void setDebugOutputEnabled(bool enabled);

  int info(std::string format, ...);
  int debug(std::string format, ...);
  int error(std::string format, ...);
}

namespace ssl {
  SSL_CTX *create_context();
  void configure_context(SSL_CTX *ctx, bool debug);
}

#endif