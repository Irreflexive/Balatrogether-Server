#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  #include <winsock2.h>
  #define closesocket closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #define closesocket close
#endif
#include <thread>
#include "util/logs.hpp"
#include "util/misc.hpp"
#include "server.hpp"

using namespace balatrogether;

// Construct a server object, initializating the mutex, SSL context, and config
Server::Server(int port) 
{
  logger::info << "Starting server" << std::endl;

  this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  logger::info << "Configuring socket options" << std::endl;

  this->setSocketOption(SOL_SOCKET, SO_REUSEADDR, true, "Failed to set reuse address");
  logger::info << "Allowed address reuse" << std::endl;

  this->setSocketOption(SOL_SOCKET, SO_SNDBUF, BUFFER_SIZE, "Failed to set send buffer size");
  logger::info << "Set send buffer size" << std::endl;

  this->setSocketOption(SOL_SOCKET, SO_RCVBUF, BUFFER_SIZE, "Failed to set receive buffer size");
  logger::info << "Set receive buffer size" << std::endl;

  this->setSocketOption(SOL_SOCKET, SO_KEEPALIVE, true, "Failed to set keepalive");
  this->setSocketOption(IPPROTO_TCP, TCP_KEEPIDLE, 60, "Failed to set idle timer");
  this->setSocketOption(IPPROTO_TCP, TCP_KEEPINTVL, 10, "Failed to set probe interval");
  this->setSocketOption(IPPROTO_TCP, TCP_KEEPCNT, 2, "Failed to set probe count");
  logger::info << "Enabled keepalive timer" << std::endl;

  int bindStatus = bind(this->sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindStatus < 0) {
    logger::error << "Failed to bind" << std::endl;
    exit(EXIT_FAILURE);
  }
  logger::info << "Bound to address" << std::endl;

  srand(time(NULL));
  this->config = new Config;
  this->net = new NetworkManager(this->getConfig()->isTLSEnabled(), this->getConfig()->isDebugMode());
  this->listener = new ServerEventListener(this);
  this->persistentRequests = new PersistentRequestManager;
  this->lobbies = lobby_list_t(this->getConfig()->getMaxLobbies());
  logger::debug.setEnabled(this->getConfig()->isDebugMode());

  logger::info << "Creating lobbies" << std::endl;
  for (int i = 0; i < this->getConfig()->getMaxLobbies(); i++) {
    lobbies.at(i) = new Lobby(this, i + 1);
  }
  logger::info << "Lobby state setup complete" << std::endl;
  
  if (listen(this->sockfd, 3) < 0) {
    logger::error << "Failed to listen" << std::endl;
    exit(EXIT_FAILURE);
  }
}

// Cleans up the SSL context and other state
Server::~Server()
{
  logger::info << "Shutting down server" << std::endl;
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
  logger::info << "Client from " << client->getIP() << " attempting to connect" << std::endl;
  std::thread(client_thread, this, client).detach();
}

// Returns false if the server is in progress, is full, or the player has been banned
bool Server::canConnect(client_t c)
{
  if (!this->getConfig()->isWhitelisted(c->getPlayer()->getSteamId())) return false;
  if (this->getConfig()->isBanned(c->getPlayer()->getSteamId())) return false;
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

  logger::info << "Client from " << c->getIP() << " joined server with Steam ID " << c->getPlayer()->getSteamId() << std::endl;
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

void Server::setSocketOption(int level, int option, int value, const char *err_message)
{
  if (setsockopt(this->sockfd, level, option, &value, sizeof(value)) < 0) {
    logger::error << err_message << std::endl;
    exit(EXIT_FAILURE);
  }
}

// Processes incoming client packets, executing the correct event handler
void balatrogether::client_thread(server_t server, client_t client) {
  if (!server->getNetworkManager()->handshake(client)) {
    server->lock();
    logger::error << "TLS handshake failed for " << client->getIP() << std::endl;
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
  logger::info << "Client from " << ip << " disconnected" << std::endl;
  server->unlock();
}
