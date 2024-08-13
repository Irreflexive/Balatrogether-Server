#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include <memory>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include "types.hpp"

using namespace balatrogether;

#include "client.hpp"
#include "util.hpp"

class balatrogether::Player {
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