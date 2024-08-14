#ifndef BALATROGETHER_LOBBY_LISTENER_HPP
#define BALATROGETHER_LOBBY_LISTENER_HPP

#include "../types.hpp"

using namespace balatrogether;

#include "../lobby.hpp"
#include "../listener.hpp"

class balatrogether::LobbyEventListener : public EventListener<lobby_t> {
  public:
    LobbyEventListener(lobby_t lobby);
    void client_error(lobby_t lobby, client_t client, json req, client_exception& e);
};

#endif