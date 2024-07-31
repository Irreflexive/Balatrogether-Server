#include "server.hpp"

// Constructs a JSON object that is interpreted as valid by the client
json success(string cmd, json data)
{
  json res;
  res["success"] = true;
  res["cmd"] = cmd;
  res["data"] = data;
  return res;
}

// Constructs a JSON object for a successful operation with no data
json success(string cmd)
{
  return success(cmd, json::object());
}

// Constructs a JSON object that will make the client disconnect
json error(string msg)
{
  json res;
  res["success"] = false;
  res["error"] = msg;
  return res;
}

// Construct a server object, initializating the mutex, SSL context, and config
Server::Server() 
{
  pthread_mutex_init(&this->mutex, nullptr);
  this->game.inGame = false;
  this->game.versus = false;
  srand(time(NULL));
  if (this->config.isTLSEnabled()) {
    this->ssl_ctx = create_context();
    configure_context(this->ssl_ctx, this->config.isDebugMode());
  }
}

// Cleans up the SSL context and remaining players
Server::~Server()
{
  if (this->config.isTLSEnabled()) SSL_CTX_free(this->ssl_ctx);
  for (Player* p : this->players) {
    delete p;
  }
}

// Completes the SSL handshake, returning true if it was successful
bool Server::handshake(Player* p)
{
  if (!this->config.isTLSEnabled()) return true;
  SSL *ssl = SSL_new(this->ssl_ctx);
  if (SSL_set_fd(ssl, p->getFd()) != 1) {
    return false;
  }
  p->setSSL(ssl);
  return SSL_accept(ssl) == 1;
}

// Connects a player to the server if they are able
void Server::join(Player* p)
{
  if (this->canJoin(p)) {
    std::cout << "Player " << p->getSteamId() << " joining" << std::endl;
    this->players.push_back(p);

    this->broadcast(success("JOIN", this->toJSON()));
  } else {
    this->sendToPlayer(p, error("Cannot join this server"));
  }
}

// Disconnects a player from the server
void Server::disconnect(Player* p) {
  if (this->hasAlreadyJoined(p)) {
    std::cout << "Player " << p->getSteamId() << " disconnecting" << std::endl;
    this->eliminate(p);
    for (int i = 0; i < this->players.size(); i++) {
      if (this->players[i] == p) {
        this->players.erase(this->players.begin() + i);
        if (this->players.size() == 0) this->stop();
        this->broadcast(success("LEAVE", this->toJSON()));
        delete p;
        return;
      }
    }
  }
}

// Returns true if a player has already joined the server
bool Server::hasAlreadyJoined(Player* p)
{
  for (Player* player : this->players) {
    if (player->getSteamId() == p->getSteamId()) return true;
  }
  return false;
}

// Returns false if the server is in progress, is full, or the player has been banned
bool Server::canJoin(Player* p)
{
  if (this->isRunning()) return false;
  if (this->config.getMaxPlayers() <= this->players.size()) return false;
  if (this->config.isBanned(p)) return false;
  if (this->hasAlreadyJoined(p)) return false;
  return true;
}

// Sends a JSON object to the player
void Server::sendToPlayer(Player* receiver, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = payload.dump();
  strncpy(buffer, jsonString.c_str(), BUFFER_SIZE - 2);
  buffer[jsonString.size()] = '\n';
  if (this->config.isTLSEnabled() && receiver->getSSL()) {
    int s = SSL_write(receiver->getSSL(), buffer, strlen(buffer));
    if (this->config.isDebugMode() && s <= 0) std::cout << "[SSL] Send error " << SSL_get_error(receiver->getSSL(), s) << std::endl;
  } else {
    send(receiver->getFd(), buffer, strlen(buffer), 0);
  }
  if (this->config.isDebugMode()) std::cout << "[SENT - " << receiver->getSteamId() << "] " << payload.dump() << std::endl;
}

// Sends a JSON object to a random player, excluding the sender from consideration
void Server::sendToRandom(Player* sender, json payload)
{
  player_list_t remainingPlayers = this->getRemainingPlayers();
  for (int i = 0; i < remainingPlayers.size(); i++) {
    if (remainingPlayers[i] == sender) {
      remainingPlayers.erase(remainingPlayers.begin() + i);
      break;
    }
  }
  if (remainingPlayers.size() == 0) return;
  int randomIndex = rand() % remainingPlayers.size();
  Player* randomPlayer = remainingPlayers[randomIndex];
  this->sendToPlayer(randomPlayer, payload);
}

// Sends a JSON object to all players besides the sender. If ignoreEliminated is true, will not send to eliminated players
void Server::sendToOthers(Player* sender, json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (Player* player : playerList) {
    if (player->getSteamId() != sender->getSteamId()) this->sendToPlayer(player, payload);
  }
}

// Sends a JSON object to all players. If ignoreEliminated is true, will not send to eliminated players
void Server::broadcast(json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (Player* player : playerList) {
    this->sendToPlayer(player, payload);
  }
}

// Receive JSON from a player. Returns a null json object if parsing fails or the client has disconnected
json Server::receive(Player* sender) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  size_t n;
  if (this->config.isTLSEnabled() && sender->getSSL()) {
    int s = SSL_read_ex(sender->getSSL(), buffer, BUFFER_SIZE, &n);
    if (this->config.isDebugMode() && s <= 0) std::cout << "[SSL] Receive error " << SSL_get_error(sender->getSSL(), s) << std::endl;
    if (s <= 0 || n == 0) return json();
  } else {
    n = recv(sender->getFd(), buffer, BUFFER_SIZE, 0);
    if (n <= 0) return json();
  }
  try {
    if (this->config.isDebugMode()) std::cout << "[RECEIVED - " << sender->getSteamId() << "] " << buffer << std::endl;
    json req = json::parse(buffer);
    return req;
  } catch (...) {
    return json();
  }
}

// Returns true if the player is designated the host of the server. Errors if the server is empty
bool Server::isHost(Player* p) {
  return p == this->getHost();
}

// Returns the host player. Errors if the server is empty
Player* Server::getHost() {
  return players.at(0);
}

// Starts a run given a seed, deck, stake, and versus configuration
void Server::start(Player* sender, string seed, string deck, int stake, bool versus)
{
  if (this->isRunning() || !this->isHost(sender)) return;
  if (!versus) {
    bool initialized = false;
    std::string unlockHash;
    for (Player* p : this->players) {
      if (!initialized) {
        initialized = true;
        unlockHash = p->getUnlocks();
      } else if (unlockHash != p->getUnlocks()) {
        return;
      }
    }
  }

  std::cout << "Starting multiplayer run" << std::endl;
  this->game.inGame = true;
  this->game.versus = versus;
  this->game.bossPhase = false;
  this->game.eliminated = player_list_t();
  this->game.ready = player_list_t();
  this->game.scores = leaderboard_t();
  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  if (this->config.isDebugMode()) data["debug"] = true;
  this->broadcast(success("START", data));
  if (versus) this->broadcast(success("STATE_INFO", this->getState()));
}

// Stops an in-progress run. Doesn't send any packets, just allows start() to be called again
void Server::stop()
{
  if (!this->isRunning()) return;
  std::cout << "Stopping multiplayer run" << std::endl;
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

// Returns a list of remaining players. This is all connected players in co-op
player_list_t Server::getRemainingPlayers() {
  if (!this->isVersus()) return this->players;
  player_list_t remaining;
  for (Player* player : this->players) {
    bool eliminated = false;
    for (Player* eliminatedPlayer : this->game.eliminated) {
      if (player == eliminatedPlayer) {
        eliminated = true;
        break;
      }
    }
    if (!eliminated) remaining.push_back(player);
  }
  return remaining;
}

// Returns a list of eliminated players
player_list_t Server::getEliminatedPlayers() {
  return this->game.eliminated;
}

// Tells all players to commence endless mode
void Server::endless()
{
  if (!this->isCoop()) return;
  this->broadcast(success("ENDLESS"));
}

// Highlights a card in a certain card area based on its Multiplayer ID
void Server::highlight(Player* sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

// Unhighlights a card in a certain card area based on its Multiplayer ID
void Server::unhighlight(Player* sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("UNHIGHLIGHT", data));
}

// Unhighlights all cards in hand. Triggered by right click in co-op
void Server::unhighlightAll(Player* sender)
{
  if (!this->isCoop()) return;
  this->sendToOthers(sender, success("UNHIGHLIGHT_ALL"));
}

// Triggers when play hand button is selected. Plays highlighted cards in hand
void Server::playHand(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("PLAY_HAND"));
}

// Triggers when discard hand button is selected. Discards highlighted cards in hand
void Server::discardHand(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("DISCARD_HAND"));
}

// Sorts the cards in hand by rank or suit
void Server::sortHand(Player* sender, string sortType)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

// Moves card at position `from` to position `to` in a certain card area
void Server::reorder(Player* sender, string selectType, int from, int to)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["from"] = from;
  data["to"] = to;
  this->sendToOthers(sender, success("REORDER", data));
}

// Triggered when the select blind button is clicked
void Server::selectBlind(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SELECT_BLIND"));
}

// Triggered when the skip blind button is clicked
void Server::skipBlind(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BLIND"));
}

// Sells a card in a certain card area based on its Multiplayer ID
void Server::sell(Player* sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("SELL", data));
}

// Uses a card in a certain card area based on its Multiplayer ID
void Server::use(Player* sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("USE", data));
}

// Buys a card in a certain card area based on its Multiplayer ID
void Server::buy(Player* sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("BUY", data));
}

// Buys and uses a card from the shop based on its Multiplayer ID
void Server::buyAndUse(Player* sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("BUY_AND_USE", data));
}

// Triggers when the skip button is clicked in a booster pack
void Server::skipBooster(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BOOSTER"));
}

// Triggers when the reroll button is clicked in the shop
void Server::reroll(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("REROLL"));
}

// Triggers when the next round button is clicked in the shop
void Server::nextRound(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("NEXT_ROUND"));
}

// Triggers when the cash out button is clicked at the end of round
void Server::goToShop(Player* sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("GO_TO_SHOP"));
}

// Initiate an exchange with a random opponent to swap jokers
void Server::swapJokers(Player* sender, json jokers)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.create(sender);
  json data;
  data["jokers"] = jokers;
  data["request_id"] = preq->getId();
  this->sendToRandom(sender, success("SWAP_JOKERS", data));
}

// Complete a previously initiated request to swap jokers, sending the opponents's jokers to the original player
void Server::swapJokers(Player* sender, json jokers, string requestId)
{
  if (!this->isVersus()) return;
  json data;
  data["jokers"] = jokers;
  PersistentRequest* preq = this->persistentRequests.getById(requestId);
  if (!preq) return;
  this->sendToPlayer(preq->getCreator(), success("SWAP_JOKERS", data));
  this->persistentRequests.complete(requestId);
}

// Changes a player's money by `change` amount
void Server::changeMoney(Player* sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToPlayer(sender, success("MONEY", data));
}

// Changes opponents' money by `change` amount
void Server::changeOthersMoney(Player* sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToOthers(sender, success("MONEY", data), true);
}

// Changes opponents (or a random player's) hand size by `change`
void Server::changeHandSize(Player* sender, int change, bool chooseRandom)
{
  if (!this->isVersus()) return;
  json data;
  data["hand_size"] = change;
  if (chooseRandom) {
    this->sendToRandom(sender, success("HAND_SIZE", data));
  } else {
    this->sendToOthers(sender, success("HAND_SIZE", data), true);
  }
}

// Initiates a request to determine all opponents' playing cards and jokers
void Server::getCardsAndJokers(Player* sender)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.create(sender);
  json preqData;
  preqData["contributed"] = json::object();
  preqData["contributed"][(string) *sender] = true;
  preqData["results"] = json::object();
  preqData["results"]["jokers"] = json::array();
  preqData["results"]["cards"] = json::array();
  preq->setData(preqData);
  json data;
  data["request_id"] = preq->getId();
  this->sendToOthers(sender, success("GET_CARDS_AND_JOKERS", data), true);
  for (Player* p : this->getRemainingPlayers()) {
    if (!preqData["contributed"][(string) *p].get<bool>()) return;
  }
  this->sendToPlayer(preq->getCreator(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
  this->persistentRequests.complete(preq->getId());
}

// An opponents' response to a request to retrieve playing cards and jokers
void Server::getCardsAndJokers(Player* sender, json jokers, json cards, string requestId)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->persistentRequests.getById(requestId);
  if (!preq) return;
  json preqData = preq->getData();
  if (preqData["contributed"][(string) *sender].get<bool>()) return;
  preqData["contributed"][(string) *sender] = true;
  for (json joker : jokers) {
    preqData["results"]["jokers"].push_back(joker);
  }
  for (json card : cards) {
    preqData["results"]["cards"].push_back(card);
  }
  preq->setData(preqData);
  for (Player* p : this->getRemainingPlayers()) {
    if (!preqData["contributed"][(string) *p].get<bool>()) return;
  }
  this->sendToPlayer(preq->getCreator(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
  this->persistentRequests.complete(preq->getId());
}

// Triggers when the player has selected a versus boss blind. Starts the boss when all remaining players are ready
void Server::readyForBoss(Player* sender)
{
  if (!this->isVersus()) return;
  if (this->game.bossPhase) return;
  for (Player* player : this->game.ready) {
    if (player->getSteamId() == sender->getSteamId()) return;
  }
  this->game.ready.push_back(sender);
  for (Player* player : this->getRemainingPlayers()) {
    bool isReady = false;
    for (Player* readyPlayer : this->game.ready) {
      if (player->getSteamId() == readyPlayer->getSteamId()) {
        isReady = true;
        break;
      }
    }
    if (!isReady) return;
  }
  this->game.bossPhase = true;
  this->game.ready = player_list_t();
  this->game.scores = leaderboard_t();
  this->broadcast(success("START_BOSS"), true);
}

// Eliminates a player from the run. If only one remains after, they are declared the winner
void Server::eliminate(Player* p)
{
  if (!this->isVersus()) return;
  for (Player* player : this->game.eliminated) {
    if (player->getSteamId() == p->getSteamId()) return;
  }
  this->game.eliminated.push_back(p);
  for (int i = 0; i < this->game.scores.size(); i++) {
    player_score_t pair = this->game.scores[i];
    if (pair.first == p) {
      this->game.scores.erase(this->game.scores.begin() + i);
      break;
    }
  }
  player_list_t remainingPlayers = this->getRemainingPlayers();
  if (remainingPlayers.size() == 1) {
    Player* winner = remainingPlayers.at(0);
    this->sendToPlayer(winner, success("WIN"));
  } else {
    this->broadcast(success("STATE_INFO", this->getState()));
  }
}

void Server::defeatedBoss(Player* p, double score)
{
  if (!this->isVersus()) return;
  if (!this->game.bossPhase) return;
  for (player_score_t pair : this->game.scores) {
    if (pair.first == p) return;
  }
  this->game.scores.push_back(player_score_t(p, score));
  if (this->game.scores.size() == this->getRemainingPlayers().size()) {
    std::sort(this->game.scores.begin(), this->game.scores.end(), [](player_score_t a, player_score_t b) {
      return a.second > b.second;
    });
    json data;
    data["leaderboard"] = json::array();
    for (player_score_t pair : this->game.scores) {
      json row;
      row["player"] = (string) *(pair.first);
      row["score"] = pair.second;
      data["leaderboard"].push_back(row);
    }
    player_list_t eliminatedPlayers = this->getEliminatedPlayers();
    std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
    for (Player* player : eliminatedPlayers) {
      json row;
      row["player"] = (string) *player;
      data["leaderboard"].push_back(row);
    }
    this->game.bossPhase = false;
    this->broadcast(success("LEADERBOARD", data), true);
  }
}

json Server::getState() {
  json state;
  state["remaining"] = this->getRemainingPlayers().size();
  return state;
}

json Server::toJSON() {
  json server;
  server["maxPlayers"] = this->config.getMaxPlayers();
  server["inGame"] = this->isRunning();

  json playerIds = json::array();
  for (Player* player : this->players) {
    playerIds.push_back((string) *player);
  }
  server["players"] = playerIds;

  return server;
}

void Server::lock()
{
  pthread_mutex_lock(&this->mutex);
}

void Server::unlock()
{
  pthread_mutex_unlock(&this->mutex);
}
