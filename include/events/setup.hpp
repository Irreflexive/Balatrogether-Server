#ifndef BALATROGETHER_SERVER_EVENTS_H
#define BALATROGETHER_SERVER_EVENTS_H

#include "../network.hpp"
#include "../server.hpp"

using namespace balatrogether;

// Triggered when a player wants to join the server, exchanging their steam ID and unlock hash
class JoinEvent : public NetworkEvent<server_t> {
  public:
    JoinEvent() : NetworkEvent("JOIN") {};
    virtual void execute(server_t server, client_t client, json req);
};

// Triggered when the lobby host wants to start a run
class StartRunEvent : public NetworkEvent<lobby_t> {
  public:
    StartRunEvent() : NetworkEvent("START") {};
    virtual void execute(lobby_t lobby, client_t client, json req);
};

#endif