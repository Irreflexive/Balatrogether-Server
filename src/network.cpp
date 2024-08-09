#include "logs.hpp"
#include "network.hpp"

// Sends a JSON object to the player
void NetworkManager::sendTo(client_list_t receivers, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = payload.dump();
  strncpy(buffer, jsonString.c_str(), BUFFER_SIZE - 2);
  buffer[jsonString.size()] = '\n';
  for (client_t receiver : receivers) {
    if (this->ssl_ctx && receiver->getSSL()) {
      int s = SSL_write(receiver->getSSL(), buffer, strlen(buffer));
      if (s <= 0) logger::debugLog("SSL send error %d", SSL_get_error(receiver->getSSL(), s));
    } else {
      send(receiver->getFd(), buffer, strlen(buffer), 0);
    }
    logger::debugLog("Server -> %s: %s", receiver->getIdentity().c_str(), payload.dump().c_str());
  }
}

// Receive JSON from a player. Returns a null json object if parsing fails or the client has disconnected
json NetworkManager::receiveFrom(client_t sender)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  size_t n;
  if (this->ssl_ctx && sender->getSSL()) {
    int s = SSL_read_ex(sender->getSSL(), buffer, BUFFER_SIZE, &n);
    if (s <= 0) logger::debugLog("SSL receive error %d", SSL_get_error(sender->getSSL(), s));
    if (s <= 0 || n == 0) return json();
  } else {
    n = recv(sender->getFd(), buffer, BUFFER_SIZE, 0);
    if (n <= 0) return json();
  }
  try {
    logger::debugLog("%s -> Server: %s", sender->getIdentity().c_str(), buffer);
    json req = json::parse(buffer);
    return req;
  } catch (...) {
    return json();
  }
}
