#ifndef BALATROGETHER_NETWORK_H
#define BALATROGETHER_NETWORK_H

#include "types.hpp"
#include "util/ssl.hpp"
#include "player.hpp"

#define BUFFER_SIZE 65536

namespace balatrogether {
  class NetworkManager {
    public:
      NetworkManager(bool ssl, bool outputKey);
      ~NetworkManager();
      bool handshake(client_t c);
      void send(client_list_t receivers, json payload);
      json receive(client_t sender);
    private:
      size_t peek(client_t client, char* buffer, size_t bytes);
      size_t read(client_t client, char* buffer, size_t bytes);
      size_t write(client_t client, char* buffer, size_t bytes);
      SSL_CTX* ssl_ctx = nullptr;
  };
}

#endif