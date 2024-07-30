#ifndef BALATROGETHER_CONFIG_H
#define BALATROGETHER_CONFIG_H

#include "json.hpp"
#include "player.hpp"

using json = nlohmann::json;

class Config {
  public:
    Config();
    ~Config();

    int getMaxPlayers();
    void setMaxPlayers(int maxPlayers);
    bool isTLSEnabled();
    void setTLSEnabled(bool tlsEnabled);
    bool isDebugMode();
    void setDebugMode(bool debugMode);
    bool isBanned(Player* p);
    void ban(Player* p);
    void unban(Player* p);
  private:
    void save();
    int maxPlayers = 8;
    bool tlsEnabled = true;
    std::vector<std::string> banned;
    bool debugMode = false;
};

#endif