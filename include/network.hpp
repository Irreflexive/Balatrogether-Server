#ifndef BALATROGETHER_NETWORK_H
#define BALATROGETHER_NETWORK_H

#include "preq.hpp"
#include "player.hpp"

#define BUFFER_SIZE 65536

using std::string;
using json = nlohmann::json;

class NetworkManager {
  public:
    void send(client_list_t receivers, json payload);
    json receive(client_t sender);
  protected:
    SSL_CTX* ssl_ctx = nullptr;
};

#endif