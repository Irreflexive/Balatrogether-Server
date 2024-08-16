#ifndef BALATROGETHER_CONFIG_H
#define BALATROGETHER_CONFIG_H

#include "types.hpp"

namespace balatrogether {
  typedef std::vector<steamid_t> steamid_list_t;

  class Config {
    public:
      Config();
      ~Config();

      int getMaxPlayers();
      int getMaxLobbies();
      bool isTLSEnabled();
      bool isDebugMode();
      void setWhitelistEnabled(bool whitelistEnabled);

      bool isBanned(steamid_t steamId);
      void ban(steamid_t steamId);
      void unban(steamid_t steamId);

      bool isWhitelisted(steamid_t steamId);
      void whitelist(steamid_t steamId);
      void unwhitelist(steamid_t steamId);

      bool isSteamApiEnabled();
      string getSteamApiKey();
    private:
      void save();
      int maxPlayers = 8;
      int maxLobbies = 1;
      bool tlsEnabled = true;
      steamid_list_t banned;
      bool debugMode = DEBUG;
      bool whitelistEnabled = false;
      steamid_list_t whitelisted;
      bool steamApiEnabled = false;
      string steamApiKey;
  };
};

#endif