#ifndef BALATROGETHER_SERVER_EVENTS_H
#define BALATROGETHER_SERVER_EVENTS_H

#include "network.hpp"
#include "server.hpp"
#include "listener.hpp"

namespace balatrogether {
  // Triggered when a player wants to join the server, exchanging their steam ID and unlock hash
  class JoinEvent : public Event<server_t> {
    public:
      JoinEvent() : Event("JOIN") {};
      virtual void execute(server_t server, client_t client, json req);
  };

  // Triggered when a player wants to join a lobby
  class JoinLobbyEvent : public Event<server_t> {
    public:
      JoinLobbyEvent() : Event("JOIN_LOBBY") {};
      virtual void execute(server_t server, client_t client, json req);
  };

  // Triggered when the lobby host wants to start a run
  class StartRunEvent : public Event<lobby_t> {
    public:
      StartRunEvent() : Event("START") {};
      virtual void execute(lobby_t lobby, client_t client, json req);
  };
}

#endif