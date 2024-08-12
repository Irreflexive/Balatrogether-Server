#include "util.hpp"
#include "server.hpp"
#include "events/setup.hpp"

// Construct a server object, initializating the mutex, SSL context, and config
Server::Server(int port) 
{
  srand(time(NULL));
  this->config = new Config;
  this->net = new NetworkManager(this->getConfig()->isTLSEnabled(), this->getConfig()->isDebugMode());
  this->listener = new EventListener<server_t>(this);
  this->persistentRequests = new PersistentRequestManager;
  this->lobbies = lobby_list_t(this->getConfig()->getMaxLobbies());
  for (int i = 0; i < this->getConfig()->getMaxLobbies(); i++) {
    lobbies.at(i) = new Lobby(this, i + 1);
  }

  logger::setDebugOutputEnabled(this->getConfig()->isDebugMode());
  logger::info("Starting server");

  this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  logger::info("Configuring socket options");

  int opt = 1;
  if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    logger::error("Failed to set reuse address");
    exit(EXIT_FAILURE);
  }
  logger::info("Allowed address reuse");

  int send_size = BUFFER_SIZE;
  if (setsockopt(this->sockfd, SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size)) < 0) {
    logger::error("Failed to set send buffer size");
    exit(EXIT_FAILURE);
  }
  logger::info("Set send buffer size");

  int recv_size = BUFFER_SIZE;
  if (setsockopt(this->sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size)) < 0) {
    logger::error("Failed to set receive buffer size");
    exit(EXIT_FAILURE);
  }
  logger::info("Set receive buffer size");

  int keepalive = 1;
  if (setsockopt(this->sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
    logger::error("Failed to set keepalive");
    exit(EXIT_FAILURE);
  }
  logger::info("Enabled keepalive timer");

  int bindStatus = bind(this->sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindStatus < 0) {
    logger::error("Failed to bind");
    exit(EXIT_FAILURE);
  }
  logger::info("Bound to address");
  
  if (listen(this->sockfd, 3) < 0) {
    logger::error("Failed to listen");
    exit(EXIT_FAILURE);
  }

  this->listener->add(new JoinEvent);

  logger::info("Registered events");
}

// Cleans up the SSL context and other state
Server::~Server()
{
  logger::info("Shutting down server");
  for (client_t c : this->clients) {
    this->disconnect(c);
  }
  for (lobby_t lobby : this->lobbies) {
    delete lobby;
  }
  delete this->config;
  delete this->net;
  delete this->listener;
  delete this->persistentRequests;
  closesocket(this->sockfd);
}

// Accepts an incoming TCP connection and spawns the client's processing thread
void Server::acceptClient()
{
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  int clientfd = accept(this->sockfd, (struct sockaddr*) &cli_addr, &cli_len);
  client_t client = new Client(clientfd, cli_addr);
  logger::info("Client from %s attempting to connect", client->getIP().c_str());
  std::thread(client_thread, this, client).detach();
}

// Returns false if the server is in progress, is full, or the player has been banned
bool Server::canConnect(client_t c)
{
  if (!this->getConfig()->isWhitelisted(c->getPlayer())) return false;
  if (this->getConfig()->isBanned(c->getPlayer())) return false;
  for (client_t client : this->clients) {
    if (client->getIdentity() == c->getIdentity()) return false;
  }
  return true;
}

// Connects a player to the server if they are able
void Server::connect(client_t c, steamid_t steamId, string unlockHash)
{
  if (!c->getPlayer()) c->setPlayer(std::make_shared<Player>(steamId, unlockHash));
  if (!this->canConnect(c)) throw client_exception("Cannot connect to server", true);

  logger::info("Client from %s joined server with Steam ID %s", c->getIP().c_str(), c->getPlayer()->getSteamId().c_str());
  this->clients.push_back(c);
}

// Disconnects a player from the server
void Server::disconnect(client_t c) {
  for (int i = 0; i < this->clients.size(); i++) {
    if (this->clients[i] == c) {
      this->clients.erase(this->clients.begin() + i);
      lobby_t lobby = c->getLobby();
      if (lobby) lobby->remove(c);
      c->setPlayer(nullptr);
      delete c;
      break;
    }
  }
}

lobby_t Server::getDefaultLobby()
{
  if (this->getLobbies().size() != 1) return nullptr;
  return this->lobbies.at(0);
}

lobby_t Server::getLobby(int index)
{
  if (index <= 0 || index > this->getLobbies().size()) return nullptr;
  return this->lobbies.at(index - 1);
}

lobby_list_t Server::getLobbies()
{
  return this->lobbies;
}

// Locks the mutex
void Server::lock()
{
  this->mutex.lock();
}

// Unlocks the mutex
void Server::unlock()
{
  this->mutex.unlock();
}

client_list_t Server::getClients()
{
  return this->clients;
}

// Returns the internal network manager for the server
network_t Server::getNetworkManager()
{
  return this->net;
}

// Returns the server config
config_t Server::getConfig()
{
  return this->config;
}

// Returns the network event listener object
server_listener_t Server::getEventListener()
{
  return this->listener;
}

// Returns the persistent request manager
preq_manager_t Server::getPersistentRequestManager()
{
  return this->persistentRequests;
}

// Processes incoming client packets, executing the correct event handler
void client_thread(server_t server, client_t client) {
  if (!server->getNetworkManager()->handshake(client)) {
    server->lock();
    logger::error("TLS handshake failed for %s", client->getIP().c_str());
    server->disconnect(client);
    server->unlock();
    return;
  }

  while (true) {
    json req = server->getNetworkManager()->receive(client);
    if (req == json() || !req["cmd"].is_string()) break;

    server->lock();
    bool success = false;
    lobby_t lobby = client->getLobby();
    if (lobby) { 
      success = lobby->getEventListener()->process(client, req);
    } else {
      success = server->getEventListener()->process(client, req);
    }
    server->unlock();

    if (!success) break;
  }

  server->lock();
  string ip = client->getIP();
  server->disconnect(client);
  logger::info("Client from %s disconnected", ip.c_str());
  server->unlock();
}
