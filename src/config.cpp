#include "util.hpp"
#include "config.hpp"

Config::Config()
{
  FILE* config_file = fopen((getpath() + "/config.json").c_str(), "r");
  if (config_file) {
    json config = json::parse(config_file);
    if (config["max_players"].is_number_integer()) this->maxPlayers = config["max_players"].get<int>();
    if (config["max_lobbies"].is_number_integer()) this->maxLobbies = config["max_lobbies"].get<int>();
    if (config["tls_enabled"].is_boolean()) this->tlsEnabled = config["tls_enabled"].get<bool>();
    if (config["banned_users"].is_array()) this->banned = config["banned_users"].get<steamid_list_t>();
    if (config["debug_mode"].is_boolean()) this->debugMode = config["debug_mode"].get<bool>();
    if (config["whitelist_enabled"].is_boolean()) this->whitelistEnabled = config["whitelist_enabled"].get<bool>();
    if (config["whitelist"].is_array()) this->whitelisted = config["whitelist"].get<steamid_list_t>();
    fclose(config_file);
    logger::info << "Config loaded from config.json" << std::endl;
  } else {
    logger::info << "No config file found, creating default config" << std::endl;
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

int Config::getMaxLobbies()
{
  return this->maxLobbies;
}

void Config::setMaxLobbies(int maxLobbies)
{
  this->maxLobbies = maxLobbies;
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

void Config::setWhitelistEnabled(bool whitelistEnabled)
{
  this->whitelistEnabled = whitelistEnabled;
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

bool Config::isWhitelisted(player_t p)
{
  return this->isWhitelisted(p->getSteamId());
}

bool Config::isWhitelisted(steamid_t steamId)
{
  if (!this->whitelistEnabled) return true;
  for (steamid_t id : this->whitelisted) {
    if (id == steamId) return true;
  }
  return false;
}

void Config::whitelist(player_t p)
{
  this->whitelist(p->getSteamId());
}

void Config::whitelist(steamid_t steamId)
{
  if (this->isWhitelisted(steamId)) return;
  this->whitelisted.push_back(steamId);
  save();
}

void Config::unwhitelist(player_t p)
{
  this->unwhitelist(p->getSteamId());
}

void Config::unwhitelist(steamid_t steamId)
{
  for (int i = 0; i < this->whitelisted.size(); i++) {
    steamid_t id = this->whitelisted.at(i);
    if (id == steamId) {
      this->whitelisted.erase(this->whitelisted.begin() + i);
      save();
      return;
    }
  }
}

void Config::save()
{
  FILE* config_file = fopen((getpath() + "/config.json").c_str(), "w");
  json config = {
    {"max_players", this->maxPlayers},
    {"max_lobbies", this->maxLobbies},
    {"tls_enabled", this->tlsEnabled},
    {"banned_users", this->banned},
    {"debug_mode", this->debugMode},
    {"whitelist_enabled", this->whitelistEnabled},
    {"whitelist", this->whitelisted}
  };
  fprintf(config_file, config.dump(2).c_str());
  fclose(config_file);
}
