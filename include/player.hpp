#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include <memory>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include "types.hpp"

using namespace Balatrogether;

#include "lobby.hpp"
#include "util.hpp"

class Balatrogether::Client {
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

class Balatrogether::Player {
  public:
    Player(steamid_t steamId, string unlockHash);

    client_t getClient();
    steamid_t getSteamId();
    string getUnlocks();
  private:
    friend class Client;
    steamid_t steamId;
    string unlockHash;
    client_t client = nullptr;
};

#endif