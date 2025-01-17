#ifndef BALATROGETHER_LOBBY_H
#define BALATROGETHER_LOBBY_H

#include "types.hpp"
#include "util/logs.hpp"
#include "game.hpp"
#include "listeners/lobby.hpp"

namespace balatrogether {
  class Lobby {
    public:
      Lobby(server_t server, int roomNumber);
      ~Lobby();

      bool canJoin(client_t client);
      bool isHost(client_t client);
      client_t getHost();

      void sendToPlayer(client_t client, json payload);
      void sendToOthers(client_t client, json payload, bool ignoreEliminated = false);
      void broadcast(json payload, bool ignoreEliminated = false);

      void add(client_t client);
      void remove(client_t client);
      void close();

      int getRoomNumber();
      server_t getServer();
      lobby_listener_t getEventListener();
      client_list_t getClients();
      game_t getGame();
      logger::stream getLogger();
      json getJSON();
    private:
      int roomNumber;
      server_t server;
      lobby_listener_t listener;
      client_list_t clients;
      logger::stream logger;
      game_t game;
  };
}

#endif