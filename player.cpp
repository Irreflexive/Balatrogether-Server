#include "player.hpp"

Player::Player(int fd, sockaddr_in addr)
{
  this->fd = fd;
  this->addr = addr;
}

Player::~Player()
{
  close(this->fd);
  if (this->ssl) {
    SSL_shutdown(this->ssl);
    SSL_free(this->ssl);
    this->ssl = nullptr;
  }
}

int Player::getFd()
{
  return this->fd;
}

steamid_t Player::getSteamId()
{
  return this->steamId;
}

void Player::setSteamId(steamid_t steamId)
{
  this->steamId = steamId;
}

std::string Player::getUnlocks()
{
  return this->unlockHash;
}

void Player::setUnlocks(std::string unlockHash)
{
  this->unlockHash = unlockHash;
}

SSL *Player::getSSL()
{
  return this->ssl;
}

void Player::setSSL(SSL *ssl)
{
  this->ssl = ssl;
}