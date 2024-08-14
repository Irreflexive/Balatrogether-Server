#ifndef BALATROGETHER_LOBBY_LISTENER_HPP
#define BALATROGETHER_LOBBY_LISTENER_HPP

#include "types.hpp"
#include "listener.hpp"

namespace balatrogether {
  class LobbyEventListener : public EventListener<lobby_t> {
    public:
      LobbyEventListener(lobby_t lobby);
      void client_error(lobby_t lobby, client_t client, json req, client_exception& e);
  };
}

#endif