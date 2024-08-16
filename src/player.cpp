#include "player.hpp"
#include "util/logs.hpp"
#include "httplib.h"

using namespace balatrogether;

Player::Player(player_auth auth)
{
  this->auth = auth;
  if (auth.fetchKey.size() > 0) {
    httplib::Client http("http://api.steampowered.com");
    httplib::Result res = http.Get("/ISteamUser/GetPlayerSummaries/v0002/?key=" + auth.fetchKey + "&steamids=" + auth.steamId);
    if (res && res->status >= 200 && res->status < 300) {
      json body = json::parse(res->body);
      this->name = body["response"]["players"][0]["personaname"];
      logger::debug << "Steam ID " << this->getSteamId() << " associated with username " << this->getName() << std::endl;
    }
  }
}

client_t Player::getClient()
{
  return this->client;
}

string Player::getName()
{
  return this->name;
}

steamid_t Player::getSteamId()
{
  return this->auth.steamId;
}

string Player::getUnlocks()
{
  return this->auth.unlockHash;
}

int Player::getUnlockedStake(string deck)
{
  try {
    return this->auth.stakes.at(deck);
  } catch (std::out_of_range& e) {
    return -1;
  }
}
