#include "util/logs.hpp"
#include "util/misc.hpp"
#include "config.hpp"

using namespace balatrogether;

Config::Config()
{
  FILE* config_file = fopen((getpath() + "/config.json").c_str(), "r");
  if (config_file) {
    json config = json::parse(config_file);
    if (config["max_players"].is_number_integer()) 
      this->maxPlayers = config["max_players"].get<int>();
    if (config["max_lobbies"].is_number_integer()) 
      this->maxLobbies = config["max_lobbies"].get<int>();
    if (config["tls_enabled"].is_boolean()) 
      this->tlsEnabled = config["tls_enabled"].get<bool>();
    if (config["banned_users"].is_array()) 
      this->banned = config["banned_users"].get<steamid_list_t>();
    if (config["whitelist_enabled"].is_boolean()) 
      this->whitelistEnabled = config["whitelist_enabled"].get<bool>();
    if (config["whitelist"].is_array()) 
      this->whitelisted = config["whitelist"].get<steamid_list_t>();
    if (config["steam_api_key"].is_null()) 
      this->steamApiEnabled = false;
    if (config["steam_api_key"].is_string()) { 
      this->steamApiEnabled = true;
      this->steamApiKey = config["steam_api_key"].get<string>();
    }
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

int Config::getMaxLobbies()
{
  return this->maxLobbies;
}

bool Config::isTLSEnabled()
{
  return this->tlsEnabled;
}

bool Config::isDebugMode()
{
  return this->debugMode;
}

void Config::setWhitelistEnabled(bool whitelistEnabled)
{
  this->whitelistEnabled = whitelistEnabled;
  save();
}

bool Config::isBanned(steamid_t steamId)
{
  for (steamid_t id : this->banned) {
    if (id == steamId) return true;
  }
  return false;
}

void Config::ban(steamid_t steamId)
{
  if (this->isBanned(steamId)) return;
  this->banned.push_back(steamId);
  save();
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

bool Config::isWhitelisted(steamid_t steamId)
{
  if (!this->whitelistEnabled) return true;
  for (steamid_t id : this->whitelisted) {
    if (id == steamId) return true;
  }
  return false;
}

void Config::whitelist(steamid_t steamId)
{
  if (this->isWhitelisted(steamId)) return;
  this->whitelisted.push_back(steamId);
  save();
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

bool Config::isSteamApiEnabled()
{
  return this->steamApiEnabled;
}

string Config::getSteamApiKey()
{
  return this->steamApiKey;
}

void Config::save()
{
  FILE* config_file = fopen((getpath() + "/config.json").c_str(), "w");
  json config = {
    {"max_players", this->maxPlayers},
    {"max_lobbies", this->maxLobbies},
    {"tls_enabled", this->tlsEnabled},
    {"banned_users", this->banned},
    {"whitelist_enabled", this->whitelistEnabled},
    {"whitelist", this->whitelisted}
  };
  if (this->steamApiEnabled) {
    config["steam_api_key"] = this->steamApiKey;
  } else {
    config["steam_api_key"] = json();
  }
  fprintf(config_file, config.dump(2).c_str());
  fclose(config_file);
}
