#include "player.hpp"

Client::Client(int fd, sockaddr_in addr)
{
  this->fd = fd;
  this->addr = addr;
}

Client::~Client()
{
  close(this->fd);
  if (this->ssl) {
    SSL_shutdown(this->ssl);
    SSL_free(this->ssl);
    this->ssl = nullptr;
  }
  this->setPlayer(nullptr);
}

int Client::getFd()
{
  return this->fd;
}

std::string Client::getIP()
{
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(this->addr.sin_addr), ip, INET_ADDRSTRLEN);
  return std::string(ip);
}

SSL *Client::getSSL()
{
  return this->ssl;
}

void Client::setSSL(SSL *ssl)
{
  this->ssl = ssl;
}

player_t Client::getPlayer()
{
  return this->player;
}

Player::Player(steamid_t steamId, std::string unlockHash)
{
  this->steamId = steamId;
  this->unlockHash = unlockHash;
}

void Client::setPlayer(player_t player)
{
  this->player = player;
  this->player->client = this;
}

client_t Player::getClient()
{
  return this->client;
}

steamid_t Player::getSteamId()
{
  return this->steamId;
}

std::string Player::getUnlocks()
{
  return this->unlockHash;
}