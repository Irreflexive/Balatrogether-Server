#ifndef BALATROGETHER_CONFIG_H
#define BALATROGETHER_CONFIG_H

#include "types.hpp"

using namespace balatrogether;

typedef std::vector<steamid_t> steamid_list_t;

class balatrogether::Config {
  public:
    Config();
    ~Config();

    int getMaxPlayers();
    void setMaxPlayers(int maxPlayers);
    int getMaxLobbies();
    void setMaxLobbies(int maxLobbies);
    bool isTLSEnabled();
    void setTLSEnabled(bool tlsEnabled);
    bool isDebugMode();
    void setDebugMode(bool debugMode);
    void setWhitelistEnabled(bool whitelistEnabled);

    bool isBanned(steamid_t steamId);
    void ban(steamid_t steamId);
    void unban(steamid_t steamId);

    bool isWhitelisted(steamid_t steamId);
    void whitelist(steamid_t steamId);
    void unwhitelist(steamid_t steamId);
  private:
    void save();
    int maxPlayers = 8;
    int maxLobbies = 1;
    bool tlsEnabled = true;
    steamid_list_t banned;
    bool debugMode = false;
    bool whitelistEnabled = false;
    steamid_list_t whitelisted;
};

#endif