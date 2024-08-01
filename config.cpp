#include "config.hpp"

Config::Config()
{
  FILE* config_file = fopen("config.json", "r");
  if (config_file) {
    json config = json::parse(config_file);
    if (config["max_players"].is_number_integer()) this->maxPlayers = config["max_players"].get<int>();
    if (config["tls_enabled"].is_boolean()) this->tlsEnabled = config["tls_enabled"].get<bool>();
    if (config["banned_users"].is_array()) this->banned = config["banned_users"].get<steamid_list_t>();
    if (config["debug_mode"].is_boolean()) this->debugMode = config["debug_mode"].get<bool>();
    fclose(config_file);
  }
  save();
}

Config::~Config()
{
  save();
}

int Config::getMaxPlayers()
{
  return this->maxPlayers;
}

void Config::setMaxPlayers(int maxPlayers)
{
  this->maxPlayers = maxPlayers;
  save();
}

bool Config::isTLSEnabled()
{
  return this->tlsEnabled;
}

void Config::setTLSEnabled(bool tlsEnabled)
{
  this->tlsEnabled = tlsEnabled;
  save();
}

bool Config::isDebugMode()
{
  return this->debugMode;
}

void Config::setDebugMode(bool debugMode)
{
  this->debugMode = debugMode;
  save();
}

bool Config::isBanned(player_t p)
{
  return this->isBanned(p->getSteamId());
}

bool Config::isBanned(steamid_t steamId)
{
  for (steamid_t id : this->banned) {
    if (id == steamId) return true;
  }
  return false;
}

void Config::ban(player_t p)
{
  this->ban(p->getSteamId());
}

void Config::ban(steamid_t steamId)
{
  if (this->isBanned(steamId)) return;
  this->banned.push_back(steamId);
  save();
}

void Config::unban(player_t p)
{
  this->unban(p->getSteamId());
}

void Config::unban(steamid_t steamId)
{
  for (int i = 0; i < this->banned.size(); i++) {
    steamid_t id = this->banned.at(i);
    if (id == steamId) {
      this->banned.erase(this->banned.begin() + i);
      save();
      return;
    }
  }
}

void Config::save()
{
  FILE* config_file = fopen("config.json", "w");
  json config;
  config["max_players"] = this->maxPlayers;
  config["tls_enabled"] = this->tlsEnabled;
  config["banned_users"] = this->banned;
  config["debug_mode"] = this->debugMode;
  fprintf(config_file, config.dump(2).c_str());
  fclose(config_file);
}
