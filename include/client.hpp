#ifndef BALATROGETHER_CLIENT_H
#define BALATROGETHER_CLIENT_H

#include <arpa/inet.h>
#include "types.hpp"
#include "util/ssl.hpp"
#include "util/logs.hpp"
#include "util/misc.hpp"
#include "lobby.hpp"
#include "player.hpp"

namespace balatrogether {
  class Client {
    public:
      Client(int fd, sockaddr_in addr);
      ~Client();

      int getFd();
      string getIdentity();
      string getIP();
      SSL *getSSL();
      void setSSL(SSL *ssl);
      player_t getPlayer();
      void setPlayer(player_t player);
      lobby_t getLobby();
      void setLobby(lobby_t lobby);
    private:
      friend class Player;
      int fd;
      struct sockaddr_in addr;
      player_t player = nullptr;
      lobby_t lobby = nullptr;
      SSL *ssl = nullptr;
  };
}

#endif