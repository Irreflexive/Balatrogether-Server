#include "player.hpp"
#include "util/logs.hpp"

using namespace balatrogether;

Player::Player(player_auth auth)
{
  this->auth = auth;
  if (auth.fetchKey.size() > 0) this->name = getplayername(auth);
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
