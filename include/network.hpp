#ifndef BALATROGETHER_NETWORK_H
#define BALATROGETHER_NETWORK_H

#include "preq.hpp"
#include "player.hpp"

#define BUFFER_SIZE 65536

using std::string;
using json = nlohmann::json;

class NetworkManager {
  public:
    NetworkManager(bool ssl, bool outputKey);
    ~NetworkManager();
    bool handshake(client_t c);
    void send(client_list_t receivers, json payload);
    json receive(client_t sender);
  private:
    SSL_CTX* ssl_ctx = nullptr;
};

typedef std::shared_ptr<NetworkManager> network_t;

#endif