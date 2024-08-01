#include "player.hpp"

Player::Player(int fd, sockaddr_in addr)
{
  this->fd = fd;
  this->addr = addr;
  this->steamId = this->getIP();
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

std::string Player::getIP()
{
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(this->addr.sin_addr), ip, INET_ADDRSTRLEN);
  return std::string(ip);
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