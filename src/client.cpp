#include <unistd.h>
#include "client.hpp"

using namespace balatrogether;

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

void Client::setPlayer(player_t player)
{
  if (this->player) this->player->client = nullptr;
  if (player) player->client = this;
  this->player = player;
}

lobby_t Client::getLobby()
{
  return this->lobby;
}

void Client::setLobby(lobby_t lobby)
{
  this->lobby = lobby;
}