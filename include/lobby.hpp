#ifndef BALATROGETHER_LOBBY_H
#define BALATROGETHER_LOBBY_H

#include "game.hpp"
#include "network.hpp"
#include "server.hpp"

class Lobby {
  public:
  private:
    Server* server;
    client_list_t clients;
    Game game;
    PersistentRequestManager persistentRequests;
};

typedef Lobby* lobby_t;
typedef std::vector<lobby_t> lobby_list_t;

#endif