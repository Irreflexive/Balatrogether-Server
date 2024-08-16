#include "player.hpp"

using namespace balatrogether;

Player::Player(player_auth auth)
{
  this->auth = auth;
}

client_t Player::getClient()
{
  return this->client;
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
