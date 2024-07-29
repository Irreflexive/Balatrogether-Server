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

uint64_t Player::getSteamId()
{
  return this->steamId;
}

void Player::setSteamId(uint64_t steamId)
{
  this->steamId = steamId;
}

SSL *Player::getSSL()
{
  return this->ssl;
}

void Player::setSSL(SSL *ssl)
{
  this->ssl = ssl;
}

Player::operator std::string() const { 
  return std::to_string(this->steamId); 
};