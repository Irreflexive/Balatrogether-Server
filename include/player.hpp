#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include "types.hpp"
#include "util/misc.hpp"

namespace balatrogether {
  class Player {
    public:
      Player(player_auth auth);

      client_t getClient();
      steamid_t getSteamId();
      string getUnlocks();
      int getUnlockedStake(string deck);
    private:
      friend class Client;
      player_auth auth;
      client_t client = nullptr;
  };
}

#endif