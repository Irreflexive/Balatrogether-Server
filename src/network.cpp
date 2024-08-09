#include "logs.hpp"
#include "encrypt.hpp"
#include "network.hpp"

NetworkManager::NetworkManager(bool ssl, bool outputKey)
{
  if (ssl) {
    this->ssl_ctx = create_context();
    configure_context(this->ssl_ctx, outputKey);
  } else {
    this->ssl_ctx = nullptr;
  }
}

NetworkManager::~NetworkManager()
{
  if (this->ssl_ctx) SSL_CTX_free(this->ssl_ctx);
}

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

// Sends a JSON object to the player
void NetworkManager::send(client_list_t receivers, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = payload.dump();
  strncpy(buffer, jsonString.c_str(), BUFFER_SIZE - 2);
  buffer[jsonString.size()] = '\n';
  for (client_t receiver : receivers) {
    if (this->ssl_ctx && receiver->getSSL()) {
      int s = SSL_write(receiver->getSSL(), buffer, strlen(buffer));
      if (s <= 0) logger::debug("SSL send error %d", SSL_get_error(receiver->getSSL(), s));
    } else {
      ::send(receiver->getFd(), buffer, strlen(buffer), 0);
    }
    logger::debug("Server -> %s: %s", receiver->getIdentity().c_str(), payload.dump().c_str());
  }
}

// Receive JSON from a player. Returns a null json object if parsing fails or the client has disconnected
json NetworkManager::receive(client_t sender)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  size_t n;
  if (this->ssl_ctx && sender->getSSL()) {
    int s = SSL_read_ex(sender->getSSL(), buffer, BUFFER_SIZE, &n);
    if (s <= 0) logger::debug("SSL receive error %d", SSL_get_error(sender->getSSL(), s));
    if (s <= 0 || n == 0) return json();
  } else {
    n = recv(sender->getFd(), buffer, BUFFER_SIZE, 0);
    if (n <= 0) return json();
  }
  try {
    logger::debug("%s -> Server: %s", sender->getIdentity().c_str(), buffer);
    json req = json::parse(buffer);
    return req;
  } catch (...) {
    return json();
  }
}
