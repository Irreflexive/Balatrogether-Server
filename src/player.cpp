#include "player.hpp"

using namespace balatrogether;

Player::Player(steamid_t steamId, string unlockHash)
{
  this->steamId = steamId;
  this->unlockHash = unlockHash;
}

client_t Player::getClient()
{
  return this->client;
}

steamid_t Player::getSteamId()
{
  return this->steamId;
}

string Player::getUnlocks()
{
  return this->unlockHash;
}