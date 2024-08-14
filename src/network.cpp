#include "network.hpp"

using namespace balatrogether;

NetworkManager::NetworkManager(bool ssl, bool outputKey)
{
  if (ssl) {
    this->ssl_ctx = ssl::create_context();
    ssl::configure_context(this->ssl_ctx, outputKey);
  } else {
    this->ssl_ctx = nullptr;
  }
  logger::info << "Network manager initialized with TLS " << (ssl ? "enabled" : "disabled") << std::endl;
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
    if (n > 0) logger::debug << "Server -> " << receiver->getIdentity() << ": " << payload.dump() << std::endl;
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
    logger::debug << sender->getIdentity() << " -> Server: " << buffer << std::endl;
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
      logger::debug << "SSL peek error " << SSL_get_error(client->getSSL(), s) << std::endl;
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
      logger::debug << "SSL read error " << SSL_get_error(client->getSSL(), s) << std::endl;
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
      logger::debug << "SSL write error " << SSL_get_error(client->getSSL(), s) << std::endl;
      n = 0;
    }
  } else {
    n = ::send(client->getFd(), buffer, bytes, 0);
  }
  return n;
}
