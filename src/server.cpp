#include "util.hpp"
#include "server.hpp"
#include "events/server.hpp"
#include "events/coop.hpp"
#include "events/versus.hpp"

// Construct a server object, initializating the mutex, SSL context, and config
Server::Server(int port) 
{
  srand(time(NULL));
  this->config = new Config;
  this->net = new NetworkManager(this->getConfig()->isTLSEnabled(), this->getConfig()->isDebugMode());
  this->listener = new EventListener<server_t>(this);
  this->persistentRequests = new PersistentRequestManager;
  this->game = new Game;

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
  this->listener->add(new StartRunEvent);

  this->listener->add(new HighlightCardEvent);
  this->listener->add(new UnhighlightCardEvent);
  this->listener->add(new UnhighlightAllEvent);
  this->listener->add(new PlayHandEvent);
  this->listener->add(new DiscardHandEvent);
  this->listener->add(new SortHandEvent);
  this->listener->add(new ReorderCardsEvent);
  this->listener->add(new SelectBlindEvent);
  this->listener->add(new SkipBlindEvent);
  this->listener->add(new SellCardEvent);
  this->listener->add(new BuyCardEvent);
  this->listener->add(new UseCardEvent);
  this->listener->add(new BuyAndUseCardEvent);
  this->listener->add(new SkipBoosterEvent);
  this->listener->add(new RerollEvent);
  this->listener->add(new NextRoundEvent);
  this->listener->add(new GoToShopEvent);
  this->listener->add(new EndlessEvent);

  this->listener->add(new SwapJokersEvent);
  this->listener->add(new TheCupEvent);
  this->listener->add(new GreenSealEvent);
  this->listener->add(new EraserEvent);
  this->listener->add(new PaintBucketEvent);
  this->listener->add(new GetCardsAndJokersEvent);
  this->listener->add(new ReadyForBossEvent);
  this->listener->add(new EliminatedEvent);
  this->listener->add(new DefeatedBossEvent);

  logger::info("Registered events");
}

// Cleans up the SSL context and other state
Server::~Server()
{
  logger::info("Shutting down server");
  for (client_t c : this->clients) {
    this->disconnect(c);
  }
  delete this->config;
  delete this->net;
  delete this->listener;
  delete this->persistentRequests;
  delete this->game;
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

// Connects a player to the server if they are able
void Server::join(client_t c)
{
  if (this->canJoin(c)) {
    logger::info("Client from %s joined server with Steam ID %s", c->getIP().c_str(), c->getPlayer()->getSteamId().c_str());
    this->clients.push_back(c);
    this->broadcast(success("JOIN", this->toJSON()));
  } else {
    this->sendToPlayer(c, error("Cannot join this server"));
  }
}

// Disconnects a player from the server
void Server::disconnect(client_t c) {
  if (this->hasAlreadyJoined(c)) {
    logger::info("Player with Steam ID %s leaving server", c->getPlayer()->getSteamId().c_str());
    this->eliminate(c->getPlayer());
    for (int i = 0; i < this->clients.size(); i++) {
      if (this->clients[i] == c) {
        this->clients.erase(this->clients.begin() + i);
        if (this->clients.size() == 0) this->stop();
        this->broadcast(success("LEAVE", this->toJSON()));
        delete c;
        return;
      }
    }
  }
  delete c;
}

// Returns true if a player has already joined the server
bool Server::hasAlreadyJoined(client_t c)
{
  for (client_t client : this->clients) {
    if (client->getPlayer()->getSteamId() == c->getPlayer()->getSteamId()) return true;
  }
  return false;
}

// Returns false if the server is in progress, is full, or the player has been banned
bool Server::canJoin(client_t c)
{
  if (this->getGame()->isRunning()) return false;
  if (this->getConfig()->getMaxPlayers() <= this->clients.size()) return false;
  if (!this->getConfig()->isWhitelisted(c->getPlayer())) return false;
  if (this->getConfig()->isBanned(c->getPlayer())) return false;
  if (this->hasAlreadyJoined(c)) return false;
  return true;
}

// Sends a JSON object to the player
void Server::sendToPlayer(client_t receiver, json payload)
{
  this->getNetworkManager()->send({receiver}, payload);
}

// Sends a JSON object to a random player, excluding the sender from consideration
void Server::sendToRandom(client_t sender, json payload)
{
  if (!this->getGame()->isVersus()) return;
  player_list_t remainingPlayers = this->getGame()->getRemaining();
  for (int i = 0; i < remainingPlayers.size(); i++) {
    if (remainingPlayers[i] == sender->getPlayer()) {
      remainingPlayers.erase(remainingPlayers.begin() + i);
      break;
    }
  }
  if (remainingPlayers.size() == 0) return;
  int randomIndex = rand() % remainingPlayers.size();
  client_t randomPlayer = remainingPlayers[randomIndex]->getClient();
  this->getNetworkManager()->send({randomPlayer}, payload);
}

// Sends a JSON object to all players besides the sender. If ignoreEliminated is true, will not send to eliminated players
void Server::sendToOthers(client_t sender, json payload, bool ignoreEliminated)
{
  client_list_t clients;
  if (this->getGame()->isRunning()) {
    player_list_t playerList = ignoreEliminated ? this->getGame()->getRemaining() : this->getGame()->getPlayers();
    client_list_t clients;
    for (player_t player : playerList) {
      if (!player->getClient()) continue;
      if (player->getSteamId() != sender->getPlayer()->getSteamId()) clients.push_back(player->getClient());
    }
  } else {
    for (client_t client : this->getClients()) {
      if (client->getPlayer()->getSteamId() != sender->getPlayer()->getSteamId()) clients.push_back(client);
    }
  }
  this->getNetworkManager()->send(clients, payload);
}

// Sends a JSON object to all players. If ignoreEliminated is true, will not send to eliminated players
void Server::broadcast(json payload, bool ignoreEliminated)
{
  client_list_t clients;
  if (this->getGame()->isRunning()) {
    player_list_t playerList = ignoreEliminated ? this->getGame()->getRemaining() : this->getGame()->getPlayers();
    for (player_t player : playerList) {
      clients.push_back(player->getClient());
    }
  } else {
    clients = this->getClients();
  }
  this->getNetworkManager()->send(this->clients, payload);
}

// Starts a run given a seed, deck, stake, and versus configuration
void Server::start(client_t client, string seed, string deck, int stake, bool versus)
{
  if (this->getGame()->isRunning() || !this->isHost(client) || (this->getClients().size() <= 1 && !this->getConfig()->isDebugMode())) return;
  if (!versus) {
    bool initialized = false;
    string unlockHash;
    for (client_t c : this->getClients()) {
      if (!initialized) {
        initialized = true;
        unlockHash = c->getPlayer()->getUnlocks();
      } else if (unlockHash != c->getPlayer()->getUnlocks()) {
        return;
      }
    }
  }

  const char* stakes[] = {
    "WHITE", 
    "RED", 
    "GREEN", 
    "BLACK", 
    "BLUE", 
    "PURPLE", 
    "ORANGE", 
    "GOLD"
  };

  logger::info("===========START RUN===========");
  logger::info("%-10s %20s", "MODE", versus ? "VERSUS" : "CO-OP");
  logger::info("%-10s %20s", "SEED", seed.c_str());
  string upperDeck = deck;
  std::transform(upperDeck.begin(), upperDeck.end(), upperDeck.begin(), [](unsigned char c){ return std::toupper(c); });
  logger::info("%-10s %20s", "DECK", upperDeck.c_str());
  logger::info("%-10s %20s", "STAKE", (stake >= 1 && stake <= 8) ? stakes[stake - 1] : "UNKNOWN");

  player_list_t players;
  for (client_t client : this->getClients()) {
    players.push_back(client->getPlayer());
  }
  this->getGame()->start(players, versus);

  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  if (this->getConfig()->isDebugMode()) data["debug"] = true;
  this->broadcast(success("START", data));
  if (versus) this->broadcast(success("STATE_INFO", this->getState()));
}

// Stops an in-progress run. Doesn't send any packets, just allows start() to be called again
void Server::stop()
{
  if (!this->getGame()->isRunning()) return;
  logger::info("============END RUN============");
  this->getGame()->reset();
}

// Returns true if the player is designated the host of the server. Errors if the server is empty
bool Server::isHost(client_t c) {
  return c == this->getHost();
}

// Returns the host player. Errors if the server is empty
client_t Server::getHost() {
  return this->getClients().at(0);
}

client_list_t Server::getClients()
{
  client_list_t clients;
  for (client_t c : this->clients) {
    if (c->getPlayer()) clients.push_back(c);
  }
  return clients;
}

// Eliminates a player from the run. If only one remains after, they are declared the winner
void Server::eliminate(player_t p)
{
  if (!this->getGame()->isVersus()) return;
  this->getGame()->eliminate(p);
  player_list_t remainingPlayers = this->getGame()->getRemaining();
  if (remainingPlayers.size() == 1) {
    player_t winner = remainingPlayers.at(0);
    this->sendToPlayer(winner->getClient(), success("WIN"));
    this->getGame()->reset();
  } else {
    this->broadcast(success("STATE_INFO", this->getState()));
  }
}

// Returns state information about the current run
json Server::getState() {
  json state;
  state["remaining"] = this->getGame()->getRemaining().size();
  return state;
}

// Returns a JSON object containing server info
json Server::toJSON() {
  json server;
  server["maxPlayers"] = this->getConfig()->getMaxPlayers();
  server["inGame"] = this->getGame()->isRunning();

  json playerIds = json::array();
  for (client_t client : this->getClients()) {
    playerIds.push_back(client->getPlayer()->getSteamId());
  }
  server["players"] = playerIds;

  return server;
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

server_listener_t Server::getEventListener()
{
  return this->listener;
}

PersistentRequestManager *Server::getPersistentRequestManager()
{
  return this->persistentRequests;
}

game_t Server::getGame()
{
  return this->game;
}

void client_thread(Server* server, client_t client) {
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
    bool success = server->getEventListener()->process(client, req);
    server->unlock();

    if (!success) break;
  }

  server->lock();
  string ip = client->getIP();
  server->disconnect(client);
  logger::info("Client from %s disconnected", ip.c_str());
  server->unlock();
}
