#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include "types.hpp"
#include "client.hpp"

namespace balatrogether {
  class Player {
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
}

#endif