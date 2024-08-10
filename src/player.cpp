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
  if (this->player) this->player->client = nullptr;
}

int Client::getFd()
{
  return this->fd;
}

string Client::getIdentity()
{
  if (this->getPlayer()) return this->getPlayer()->getSteamId();
  return this->getIP();
}

string Client::getIP()
{
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(this->addr.sin_addr), ip, INET_ADDRSTRLEN);
  return string(ip);
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

Player::Player(steamid_t steamId, string unlockHash)
{
  this->steamId = steamId;
  this->unlockHash = unlockHash;
}

void Client::setPlayer(player_t player)
{
  this->player = player;
  this->player->client = this;
}

lobby_t Client::getLobby()
{
  return this->lobby;
}

void Client::setLobby(lobby_t lobby)
{
  this->lobby = lobby;
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