#ifndef BALATROGETHER_SERVER_EVENTS_H
#define BALATROGETHER_SERVER_EVENTS_H

#include "../network.hpp"
#include "../server.hpp"

class JoinEvent : public NetworkEvent<server_t> {
  public:
    JoinEvent() : NetworkEvent("JOIN") {};
    virtual void execute(server_t server, client_t client, json req);
};

class StartRunEvent : public NetworkEvent<server_t> {
  public:
    StartRunEvent() : NetworkEvent("START") {};
    virtual void execute(server_t server, client_t client, json req);
};

#endif