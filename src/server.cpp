#include "server.hpp"

// Garbage collects persistent requests that have been alive for more than 10 seconds. Call in a separate thread
void collectRequests(Server *server)
{
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(60));
    server->lock();
    server->debugLog("Collecting PersistentRequest garbage");
    server->persistentRequests.clearUnresolved(10);
    server->unlock();
  }
}

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
  this->game.inGame = false;
  this->game.versus = false;
  srand(time(NULL));
  if (this->config.isTLSEnabled()) {
    this->ssl_ctx = create_context();
    configure_context(this->ssl_ctx, this->config.isDebugMode());
  }
  this->requestCollector = std::thread(collectRequests, this);
  this->requestCollector.detach();
}

// Cleans up the SSL context and other state
Server::~Server()
{
  this->infoLog("Shutting down server");
  if (this->config.isTLSEnabled()) SSL_CTX_free(this->ssl_ctx);
  for (player_t p : this->players) {
    this->disconnect(p);
  }
}

// Completes the SSL handshake, returning true if it was successful
bool Server::handshake(player_t p)
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
void Server::join(player_t p)
{
  if (this->canJoin(p)) {
    this->infoLog("Client from %s joined server with Steam ID %s", p->getIP().c_str(), p->getSteamId().c_str());
    this->players.push_back(p);
    this->broadcast(success("JOIN", this->toJSON()));
  } else {
    this->sendToPlayer(p, error("Cannot join this server"));
  }
}

// Disconnects a player from the server
void Server::disconnect(player_t p) {
  if (this->hasAlreadyJoined(p)) {
    this->infoLog("Player with Steam ID %s leaving server", p->getSteamId().c_str());
    this->eliminate(p);
    for (int i = 0; i < this->players.size(); i++) {
      if (this->players[i] == p) {
        this->players.erase(this->players.begin() + i);
        if (this->players.size() == 0) this->stop();
        this->broadcast(success("LEAVE", this->toJSON()));
        return;
      }
    }
  }
  delete p;
}

// Returns true if a player has already joined the server
bool Server::hasAlreadyJoined(player_t p)
{
  for (player_t player : this->players) {
    if (player->getSteamId() == p->getSteamId()) return true;
  }
  return false;
}

// Returns false if the server is in progress, is full, or the player has been banned
bool Server::canJoin(player_t p)
{
  if (this->isRunning()) return false;
  if (this->config.getMaxPlayers() <= this->players.size()) return false;
  if (!this->config.isWhitelisted(p)) return false;
  if (this->config.isBanned(p)) return false;
  if (this->hasAlreadyJoined(p)) return false;
  return true;
}

// Sends a JSON object to the player
void Server::sendToPlayer(player_t receiver, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = payload.dump();
  strncpy(buffer, jsonString.c_str(), BUFFER_SIZE - 2);
  buffer[jsonString.size()] = '\n';
  if (this->config.isTLSEnabled() && receiver->getSSL()) {
    int s = SSL_write(receiver->getSSL(), buffer, strlen(buffer));
    if (s <= 0) this->debugLog("SSL send error %d", SSL_get_error(receiver->getSSL(), s));
  } else {
    send(receiver->getFd(), buffer, strlen(buffer), 0);
  }
  this->debugLog("Server -> %s: %s", receiver->getSteamId().c_str(), payload.dump().c_str());
}

// Sends a JSON object to a random player, excluding the sender from consideration
void Server::sendToRandom(player_t sender, json payload)
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
  player_t randomPlayer = remainingPlayers[randomIndex];
  this->sendToPlayer(randomPlayer, payload);
}

// Sends a JSON object to all players besides the sender. If ignoreEliminated is true, will not send to eliminated players
void Server::sendToOthers(player_t sender, json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (player_t player : playerList) {
    if (player->getSteamId() != sender->getSteamId()) this->sendToPlayer(player, payload);
  }
}

// Sends a JSON object to all players. If ignoreEliminated is true, will not send to eliminated players
void Server::broadcast(json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (player_t player : playerList) {
    this->sendToPlayer(player, payload);
  }
}

// Receive JSON from a player. Returns a null json object if parsing fails or the client has disconnected
json Server::receive(player_t sender) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  size_t n;
  if (this->config.isTLSEnabled() && sender->getSSL()) {
    int s = SSL_read_ex(sender->getSSL(), buffer, BUFFER_SIZE, &n);
    if (s <= 0) this->debugLog("SSL receive error %d", SSL_get_error(sender->getSSL(), s));
    if (s <= 0 || n == 0) return json();
  } else {
    n = recv(sender->getFd(), buffer, BUFFER_SIZE, 0);
    if (n <= 0) return json();
  }
  try {
    this->debugLog("%s -> Server: %s", sender->getSteamId().c_str(), buffer);
    json req = json::parse(buffer);
    return req;
  } catch (...) {
    return json();
  }
}

// Returns true if the player is designated the host of the server. Errors if the server is empty
bool Server::isHost(player_t p) {
  return p == this->getHost();
}

// Returns the host player. Errors if the server is empty
player_t Server::getHost() {
  return players.at(0);
}

// Starts a run given a seed, deck, stake, and versus configuration
void Server::start(player_t sender, string seed, string deck, int stake, bool versus)
{
  if (this->isRunning() || !this->isHost(sender) || this->players.size() <= 1) return;
  if (!versus) {
    bool initialized = false;
    std::string unlockHash;
    for (player_t p : this->players) {
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

  this->infoLog("===========START RUN===========");
  this->infoLog("%-10s %20s", "MODE", versus ? "VERSUS" : "CO-OP");
  this->infoLog("%-10s %20s", "SEED", seed.c_str());
  std::string upperDeck = deck;
  std::transform(upperDeck.begin(), upperDeck.end(), upperDeck.begin(), [](unsigned char c){ return std::toupper(c); });
  this->infoLog("%-10s %20s", "DECK", upperDeck.c_str());
  this->infoLog("%-10s %20s", "STAKE", (stake >= 1 && stake <= 8) ? stakes[stake - 1] : "UNKNOWN");

  this->game.inGame = true;
  this->game.versus = versus;
  this->game.bossPhase = false;
  this->game.eliminated = steamid_list_t();
  this->game.ready = steamid_list_t();
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
  this->infoLog("============END RUN============");
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

// Returns a list of all players connected to the server
player_list_t Server::getPlayers()
{
  return this->players;
}

// Returns a list of remaining players. This is all connected players in co-op
player_list_t Server::getRemainingPlayers() {
  if (!this->isVersus()) return this->players;
  player_list_t remaining;
  for (player_t player : this->players) {
    bool eliminated = false;
    for (steamid_t eliminatedPlayer : this->game.eliminated) {
      if (player->getSteamId() == eliminatedPlayer) {
        eliminated = true;
        break;
      }
    }
    if (!eliminated) remaining.push_back(player);
  }
  return remaining;
}

// Returns a list of eliminated players by their steam ID
steamid_list_t Server::getEliminatedPlayers() {
  return this->game.eliminated;
}

// Tells all players to commence endless mode
void Server::endless()
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
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

// Unhighlights a card in a certain card area based on its Multiplayer ID
void Server::unhighlight(player_t sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("UNHIGHLIGHT", data));
}

// Unhighlights all cards in hand. Triggered by right click in co-op
void Server::unhighlightAll(player_t sender)
{
  if (!this->isCoop()) return;
  this->sendToOthers(sender, success("UNHIGHLIGHT_ALL"));
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
  this->sendToOthers(sender, success("REORDER", data));
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
    if (joker["k"].is_null()) throw "No joker key";
  }
  data["request_id"] = preq->getId();
  this->sendToRandom(sender, success("SWAP_JOKERS", data));
}

// Complete a previously initiated request to swap jokers, sending the opponents's jokers to the original player
void Server::swapJokers(player_t sender, json jokers, string requestId)
{
  if (!this->isVersus()) return;
  json data;
  data["jokers"] = jokers;
  for (json joker : jokers) {
    if (joker["k"].is_null()) throw "No joker key";
  }
  PersistentRequest* preq = this->persistentRequests.getById(requestId);
  if (!preq) return;
  this->sendToPlayer(preq->getCreator(), success("SWAP_JOKERS", data));
  this->persistentRequests.complete(requestId);
}

// Changes a player's money by `change` amount
void Server::changeMoney(player_t sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToPlayer(sender, success("MONEY", data));
}

// Changes opponents' money by `change` amount
void Server::changeOthersMoney(player_t sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToOthers(sender, success("MONEY", data), true);
}

// Changes opponents (or a random player's) hand size by `change`
void Server::changeHandSize(player_t sender, int change, bool chooseRandom)
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
  this->sendToOthers(sender, success("GET_CARDS_AND_JOKERS", data), true);
  for (player_t p : this->getRemainingPlayers()) {
    if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
  }
  this->sendToPlayer(preq->getCreator(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
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
    if (joker["k"].is_null()) throw "No joker key";
    preqData["results"]["jokers"].push_back(joker);
  }
  for (json card : cards) {
    if (card["k"].is_null()) throw "No card key";
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
  this->sendToPlayer(preq->getCreator(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
  this->persistentRequests.complete(preq->getId());
}

// Triggers when the player has selected a versus boss blind. Starts the boss when all remaining players are ready
void Server::readyForBoss(player_t sender)
{
  if (!this->isVersus()) return;
  if (this->game.bossPhase) return;
  for (steamid_t steamid : this->game.ready) {
    if (steamid == sender->getSteamId()) return;
  }
  this->game.ready.push_back(sender->getSteamId());
  for (player_t player : this->getRemainingPlayers()) {
    bool isReady = false;
    for (steamid_t readyId : this->game.ready) {
      if (player->getSteamId() == readyId) {
        isReady = true;
        break;
      }
    }
    if (!isReady) return;
  }
  this->game.bossPhase = true;
  this->game.ready = steamid_list_t();
  this->game.scores = leaderboard_t();
  this->broadcast(success("START_BOSS"), true);
}

// Eliminates a player from the run. If only one remains after, they are declared the winner
void Server::eliminate(player_t p)
{
  if (!this->isVersus()) return;
  for (steamid_t steamid : this->game.eliminated) {
    if (steamid == p->getSteamId()) return;
  }
  this->game.eliminated.push_back(p->getSteamId());
  for (int i = 0; i < this->game.scores.size(); i++) {
    score_t pair = this->game.scores[i];
    if (pair.first == p->getSteamId()) {
      this->game.scores.erase(this->game.scores.begin() + i);
      break;
    }
  }
  player_list_t remainingPlayers = this->getRemainingPlayers();
  if (remainingPlayers.size() == 1) {
    player_t winner = remainingPlayers.at(0);
    this->sendToPlayer(winner, success("WIN"));
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
    if (pair.first == p->getSteamId()) return;
  }
  this->game.scores.push_back(score_t(p->getSteamId(), score));
  if (this->game.scores.size() == this->getRemainingPlayers().size()) {
    std::sort(this->game.scores.begin(), this->game.scores.end(), [](score_t a, score_t b) {
      return a.second > b.second;
    });
    json data;
    data["leaderboard"] = json::array();
    for (score_t pair : this->game.scores) {
      json row;
      row["player"] = pair.first;
      row["score"] = pair.second;
      data["leaderboard"].push_back(row);
    }
    steamid_list_t eliminatedPlayers = this->getEliminatedPlayers();
    std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
    for (steamid_t steamId : eliminatedPlayers) {
      json row;
      row["player"] = steamId;
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
  server["maxPlayers"] = this->config.getMaxPlayers();
  server["inGame"] = this->isRunning();

  json playerIds = json::array();
  for (player_t player : this->players) {
    playerIds.push_back(player->getSteamId());
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

int Server::infoLog(std::string format, ...)
{
  std::string fmt = "[INFO] " + format;
  va_list args;
  va_start(args, format);
  int n = this->log(fmt.c_str(), args);
  va_end(args);
  return n;
}

int Server::debugLog(std::string format, ...)
{
  if (!this->config.isDebugMode()) return 0;
  std::string fmt = "[DEBUG] " + format;
  va_list args;
  va_start(args, format);
  int n = this->log(fmt.c_str(), args, "33");
  va_end(args);
  return n;
}

int Server::errorLog(std::string format, ...)
{
  if (!this->config.isDebugMode()) return 0;
  std::string fmt = "[ERROR] " + format;
  va_list args;
  va_start(args, format);
  int n = this->log(fmt.c_str(), args, "31", stderr);
  va_end(args);
  return n;
}

int Server::log(std::string format, va_list args, std::string color, FILE *fd)
{
  std::stringstream modifiedFormat;
  std::time_t currentTime = std::time(nullptr);
  modifiedFormat << "\033[" << color << "m[" << std::put_time(std::gmtime(&currentTime), "%FT%TZ") << "] " << format << "\033[0m" << std::endl;
  int n = vfprintf(fd, modifiedFormat.str().c_str(), args);
  return n;
}
