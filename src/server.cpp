#include "util.hpp"
#include "logs.hpp"
#include "server.hpp"
#include "events/server.hpp"

// Construct a server object, initializating the mutex, SSL context, and config
Server::Server(int port) 
{
  srand(time(NULL));
  this->game.inGame = false;
  this->game.versus = false;
  this->config = new Config;
  this->net = std::make_shared<NetworkManager>(this->getConfig()->isTLSEnabled(), this->getConfig()->isDebugMode());
  this->listener = new EventListener<server_t>(this);

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
  delete this->listener;
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
  if (this->isRunning()) return false;
  if (this->getConfig()->getMaxPlayers() <= this->clients.size()) return false;
  if (!this->getConfig()->isWhitelisted(c->getPlayer())) return false;
  if (this->getConfig()->isBanned(c->getPlayer())) return false;
  if (this->hasAlreadyJoined(c)) return false;
  return true;
}

// Sends a JSON object to the player
void Server::sendToPlayer(client_t receiver, json payload)
{
  this->net->send({receiver}, payload);
}

// Sends a JSON object to a random player, excluding the sender from consideration
void Server::sendToRandom(client_t sender, json payload)
{
  player_list_t remainingPlayers = this->getRemainingPlayers();
  for (int i = 0; i < remainingPlayers.size(); i++) {
    if (remainingPlayers[i] == sender->getPlayer()) {
      remainingPlayers.erase(remainingPlayers.begin() + i);
      break;
    }
  }
  if (remainingPlayers.size() == 0) return;
  int randomIndex = rand() % remainingPlayers.size();
  client_t randomPlayer = remainingPlayers[randomIndex]->getClient();
  this->net->send({randomPlayer}, payload);
}

// Sends a JSON object to all players besides the sender. If ignoreEliminated is true, will not send to eliminated players
void Server::sendToOthers(client_t sender, json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->getPlayers();
  client_list_t clients;
  for (player_t player : playerList) {
    if (player->getSteamId() != sender->getPlayer()->getSteamId()) clients.push_back(player->getClient());
  }
  this->net->send(clients, payload);
}

// Sends a JSON object to all players. If ignoreEliminated is true, will not send to eliminated players
void Server::broadcast(json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->getPlayers();
  client_list_t clients;
  for (player_t player : playerList) {
    clients.push_back(player->getClient());
  }
  this->net->send(clients, payload);
}

// Starts a run given a seed, deck, stake, and versus configuration
void Server::start(player_t sender, string seed, string deck, int stake, bool versus)
{
  if (this->isRunning() || !this->isHost(sender) || (this->clients.size() <= 1 && !this->getConfig()->isDebugMode())) return;
  if (!versus) {
    bool initialized = false;
    string unlockHash;
    for (player_t p : this->getPlayers()) {
      if (!initialized) {
        initialized = true;
        unlockHash = p->getUnlocks();
      } else if (unlockHash != p->getUnlocks()) {
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

  this->game.inGame = true;
  this->game.versus = versus;
  this->game.bossPhase = false;
  this->game.eliminated.clear();
  this->game.ready.clear();
  this->game.scores.clear();

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
  if (!this->isRunning()) return;
  logger::info("============END RUN============");
  this->game.inGame = false;
}

// Returns true if there is a run in progress
bool Server::isRunning()
{
  return this->game.inGame;
}

// Returns true if there is a versus mode run in progress
bool Server::isVersus()
{
  return this->isRunning() && this->game.versus;
}

// Returns true if there is a co-op mode run in progress
bool Server::isCoop()
{
  return this->isRunning() && !this->game.versus;
}

// Returns true if the player is designated the host of the server. Errors if the server is empty
bool Server::isHost(player_t p) {
  return p == this->getHost();
}

// Returns the host player. Errors if the server is empty
player_t Server::getHost() {
  return clients.at(0)->getPlayer();
}

// Returns a list of all players connected to the server
player_list_t Server::getPlayers()
{
  player_list_t list;
  for (client_t client : this->clients) {
    list.push_back(client->getPlayer());
  }
  return list;
}

// Returns a list of remaining players. This is all connected players in co-op
player_list_t Server::getRemainingPlayers() {
  if (!this->isVersus()) return this->getPlayers();
  player_list_t remaining;
  for (player_t player : this->getPlayers()) {
    bool eliminated = false;
    for (player_t eliminatedPlayer : this->game.eliminated) {
      if (player == eliminatedPlayer) {
        eliminated = true;
        break;
      }
    }
    if (!eliminated) remaining.push_back(player);
  }
  return remaining;
}

// Returns a list of eliminated players by their steam ID
player_list_t Server::getEliminatedPlayers() {
  return this->game.eliminated;
}

// Tells all players to commence endless mode
void Server::endless(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("ENDLESS"));
}

// Highlights a card in a certain card area based on its Multiplayer ID
void Server::highlight(player_t sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender->getClient(), success("HIGHLIGHT", data));
}

// Unhighlights a card in a certain card area based on its Multiplayer ID
void Server::unhighlight(player_t sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender->getClient(), success("UNHIGHLIGHT", data));
}

// Unhighlights all cards in hand. Triggered by right click in co-op
void Server::unhighlightAll(player_t sender)
{
  if (!this->isCoop()) return;
  this->sendToOthers(sender->getClient(), success("UNHIGHLIGHT_ALL"));
}

// Triggers when play hand button is selected. Plays highlighted cards in hand
void Server::playHand(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("PLAY_HAND"));
}

// Triggers when discard hand button is selected. Discards highlighted cards in hand
void Server::discardHand(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("DISCARD_HAND"));
}

// Sorts the cards in hand by rank or suit
void Server::sortHand(player_t sender, string sortType)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

// Moves card at position `from` to position `to` in a certain card area
void Server::reorder(player_t sender, string selectType, int from, int to)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["from"] = from;
  data["to"] = to;
  this->sendToOthers(sender->getClient(), success("REORDER", data));
}

// Triggered when the select blind button is clicked
void Server::selectBlind(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SELECT_BLIND"));
}

// Triggered when the skip blind button is clicked
void Server::skipBlind(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BLIND"));
}

// Sells a card in a certain card area based on its Multiplayer ID
void Server::sell(player_t sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("SELL", data));
}

// Uses a card in a certain card area based on its Multiplayer ID
void Server::use(player_t sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("USE", data));
}

// Buys a card in a certain card area based on its Multiplayer ID
void Server::buy(player_t sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("BUY", data));
}

// Buys and uses a card from the shop based on its Multiplayer ID
void Server::buyAndUse(player_t sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("BUY_AND_USE", data));
}

// Triggers when the skip button is clicked in a booster pack
void Server::skipBooster(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BOOSTER"));
}

// Triggers when the reroll button is clicked in the shop
void Server::reroll(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("REROLL"));
}

// Triggers when the next round button is clicked in the shop
void Server::nextRound(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("NEXT_ROUND"));
}

// Triggers when the cash out button is clicked at the end of round
void Server::goToShop(player_t sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("GO_TO_SHOP"));
}

// Initiate an exchange with a random opponent to swap jokers
void Server::swapJokers(player_t sender, json jokers)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.create(sender);
  json data;
  data["jokers"] = jokers;
  for (json joker : jokers) {
    if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
  }
  data["request_id"] = preq->getId();
  this->sendToRandom(sender->getClient(), success("SWAP_JOKERS", data));
}

// Complete a previously initiated request to swap jokers, sending the opponents's jokers to the original player
void Server::swapJokers(player_t sender, json jokers, string requestId)
{
  if (!this->isVersus()) return;
  json data;
  data["jokers"] = jokers;
  for (json joker : jokers) {
    if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
  }
  PersistentRequest* preq = this->persistentRequests.getById(requestId);
  if (!preq) return;
  this->sendToPlayer(preq->getCreator()->getClient(), success("SWAP_JOKERS", data));
  this->persistentRequests.complete(requestId);
}

// Changes a player's money by `change` amount
void Server::changeMoney(player_t sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToPlayer(sender->getClient(), success("MONEY", data));
}

// Changes opponents' money by `change` amount
void Server::changeOthersMoney(player_t sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToOthers(sender->getClient(), success("MONEY", data), true);
}

// Changes opponents (or a random player's) hand size by `change`
void Server::changeHandSize(player_t sender, int change, bool chooseRandom)
{
  if (!this->isVersus()) return;
  json data;
  data["hand_size"] = change;
  if (chooseRandom) {
    this->sendToRandom(sender->getClient(), success("HAND_SIZE", data));
  } else {
    this->sendToOthers(sender->getClient(), success("HAND_SIZE", data), true);
  }
}

// Initiates a request to determine all opponents' playing cards and jokers
void Server::getCardsAndJokers(player_t sender)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.create(sender);
  json preqData;
  preqData["contributed"] = json::object();
  preqData["contributed"][sender->getSteamId()] = true;
  preqData["results"] = json::object();
  preqData["results"]["jokers"] = json::array();
  preqData["results"]["cards"] = json::array();
  preq->setData(preqData);
  json data;
  data["request_id"] = preq->getId();
  this->sendToOthers(sender->getClient(), success("GET_CARDS_AND_JOKERS", data), true);
  for (player_t p : this->getRemainingPlayers()) {
    if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
  }
  this->sendToPlayer(preq->getCreator()->getClient(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
  this->persistentRequests.complete(preq->getId());
}

// An opponents' response to a request to retrieve playing cards and jokers
void Server::getCardsAndJokers(player_t sender, json jokers, json cards, string requestId)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.getById(requestId);
  if (!preq) return;
  json preqData = preq->getData();
  if (preqData["contributed"][sender->getSteamId()].get<bool>()) return;
  preqData["contributed"][sender->getSteamId()] = true;
  for (json joker : jokers) {
    if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
    preqData["results"]["jokers"].push_back(joker);
  }
  for (json card : cards) {
    if (card["k"].is_null()) throw std::invalid_argument("No card key");
    preqData["results"]["cards"].push_back(card);
  }
  preq->setData(preqData);
  for (player_t p : this->getRemainingPlayers()) {
    if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
  }
  json randomCards = json::array();
  for (int i = 0; i < 20; i++) {
    if (preqData["results"]["cards"].size() == 0) break;
    size_t cardIndex = rand() % preqData["results"]["cards"].size();
    randomCards.push_back(preqData["results"]["cards"].at(cardIndex));
    preqData["results"]["cards"].erase(cardIndex);
  }
  preqData["results"]["cards"] = randomCards;
  this->sendToPlayer(preq->getCreator()->getClient(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
  this->persistentRequests.complete(preq->getId());
}

// Triggers when the player has selected a versus boss blind. Starts the boss when all remaining players are ready
void Server::readyForBoss(player_t sender)
{
  if (!this->isVersus()) return;
  if (this->game.bossPhase) return;
  for (player_t player : this->game.ready) {
    if (player == sender) return;
  }
  this->game.ready.push_back(sender);
  for (player_t player : this->getRemainingPlayers()) {
    bool isReady = false;
    for (player_t ready : this->game.ready) {
      if (player == ready) {
        isReady = true;
        break;
      }
    }
    if (!isReady) return;
  }
  this->game.bossPhase = true;
  this->game.ready.clear();
  this->game.scores.clear();
  this->broadcast(success("START_BOSS"), true);
}

// Eliminates a player from the run. If only one remains after, they are declared the winner
void Server::eliminate(player_t p)
{
  if (!this->isVersus()) return;
  for (player_t player : this->game.eliminated) {
    if (player == p) return;
  }
  this->game.eliminated.push_back(p);
  for (int i = 0; i < this->game.scores.size(); i++) {
    score_t pair = this->game.scores[i];
    if (pair.first == p) {
      this->game.scores.erase(this->game.scores.begin() + i);
      break;
    }
  }
  player_list_t remainingPlayers = this->getRemainingPlayers();
  if (remainingPlayers.size() == 1) {
    player_t winner = remainingPlayers.at(0);
    this->sendToPlayer(winner->getClient(), success("WIN"));
  } else {
    this->broadcast(success("STATE_INFO", this->getState()));
  }
}

// Triggered when a player has defeated a versus boss blind. Sends the leaderboard once all players are done
void Server::defeatedBoss(player_t p, double score)
{
  if (!this->isVersus()) return;
  if (!this->game.bossPhase) return;
  for (score_t pair : this->game.scores) {
    if (pair.first == p) return;
  }
  this->game.scores.push_back(score_t(p, score));
  if (this->game.scores.size() == this->getRemainingPlayers().size()) {
    std::sort(this->game.scores.begin(), this->game.scores.end(), [](score_t a, score_t b) {
      return a.second > b.second;
    });
    json data;
    data["leaderboard"] = json::array();
    for (score_t pair : this->game.scores) {
      json row;
      row["player"] = pair.first->getSteamId();
      row["score"] = pair.second;
      data["leaderboard"].push_back(row);
    }
    player_list_t eliminatedPlayers = this->getEliminatedPlayers();
    std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
    for (player_t eliminated : eliminatedPlayers) {
      json row;
      row["player"] = eliminated->getSteamId();
      data["leaderboard"].push_back(row);
    }
    this->game.bossPhase = false;
    this->broadcast(success("LEADERBOARD", data), true);
  }
}

// Returns state information about the current run
json Server::getState() {
  json state;
  state["remaining"] = this->getRemainingPlayers().size();
  return state;
}

// Returns a JSON object containing server info
json Server::toJSON() {
  json server;
  server["maxPlayers"] = this->getConfig()->getMaxPlayers();
  server["inGame"] = this->isRunning();

  json playerIds = json::array();
  for (client_t client : this->clients) {
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
