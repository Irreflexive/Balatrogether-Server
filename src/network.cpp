#include "network.hpp"

NetworkManager::NetworkManager(bool ssl, bool outputKey)
{
  if (ssl) {
    this->ssl_ctx = ssl::create_context();
    ssl::configure_context(this->ssl_ctx, outputKey);
  } else {
    this->ssl_ctx = nullptr;
  }
}

NetworkManager::~NetworkManager()
{
  if (this->ssl_ctx) SSL_CTX_free(this->ssl_ctx);
}

// Completes the SSL handshake with a client
bool NetworkManager::handshake(client_t c)
{
  if (!this->ssl_ctx) return true;
  SSL *ssl = SSL_new(this->ssl_ctx);
  if (SSL_set_fd(ssl, c->getFd()) != 1) {
    return false;
  }
  c->setSSL(ssl);
  return SSL_accept(ssl) == 1;
}

// Sends a JSON object to a list of clients
void NetworkManager::send(client_list_t receivers, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = payload.dump();
  strncpy(buffer, jsonString.c_str(), BUFFER_SIZE - 2);
  buffer[jsonString.size()] = '\n';
  for (client_t receiver : receivers) {
    size_t n = this->write(receiver, buffer, strlen(buffer));
    if (n > 0) logger::debug("Server -> %s: %s", receiver->getIdentity().c_str(), payload.dump().c_str());
  }
}

// Receive JSON from a client. Returns a null json object if parsing fails or the client has disconnected
json NetworkManager::receive(client_t sender)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  size_t n = this->peek(sender, buffer, BUFFER_SIZE);
  size_t read_amount = 0;
  for (size_t i = 0; i < n && i < BUFFER_SIZE; i++) {
    if (buffer[i] == '\n') {
      read_amount = i + 1;
      break;
    }
  }
  if (read_amount == 0) return json();

  memset(buffer, 0, BUFFER_SIZE);
  n = this->read(sender, buffer, read_amount);
  if (n == 0) return json();

  try {
    buffer[n - 1] = '\0';
    logger::debug("%s -> Server: %s", sender->getIdentity().c_str(), buffer);
    json req = json::parse(buffer);
    return req;
  } catch (...) {
    return json();
  }
}

size_t NetworkManager::peek(client_t client, char* buffer, size_t bytes)
{
  size_t n;
  if (this->ssl_ctx && client->getSSL()) {
    int s = SSL_peek_ex(client->getSSL(), buffer, bytes, &n);
    if (s <= 0) {
      logger::debug("SSL peek error %d", SSL_get_error(client->getSSL(), s));
      n = 0;
    }
  } else {
    n = recv(client->getFd(), buffer, bytes, MSG_PEEK);
  }
  return n;
}

size_t NetworkManager::read(client_t client, char* buffer, size_t bytes)
{
  size_t n;
  if (this->ssl_ctx && client->getSSL()) {
    int s = SSL_read_ex(client->getSSL(), buffer, bytes, &n);
    if (s <= 0) {
      logger::debug("SSL read error %d", SSL_get_error(client->getSSL(), s));
      n = 0;
    }
  } else {
    n = recv(client->getFd(), buffer, bytes, 0);
  }
  return n;
}

size_t NetworkManager::write(client_t client, char* buffer, size_t bytes)
{
  size_t n;
  if (this->ssl_ctx && client->getSSL()) {
    int s = SSL_write_ex(client->getSSL(), buffer, bytes, &n);
    if (s <= 0) {
      logger::debug("SSL write error %d", SSL_get_error(client->getSSL(), s));
      n = 0;
    }
  } else {
    n = ::send(client->getFd(), buffer, bytes, 0);
  }
  return n;
}

void ServerEventListener::client_error(server_t server, client_t client, json req, client_exception &e)
{
  server->getNetworkManager()->send({client}, error(e.what()));
}

void LobbyEventListener::client_error(lobby_t lobby, client_t client, json req, client_exception &e)
{
  lobby->getServer()->getNetworkManager()->send({client}, error(e.what()));
}
