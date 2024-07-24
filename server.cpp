#include "server.hpp"

string uint64ToString(uint64_t n) {
  std::ostringstream os;
  os << n;
  return os.str();
}

bool operator==(Player const& lhs, Player const& rhs)
{
  return lhs.steamId == rhs.steamId;
}

bool operator!=(Player const &lhs, Player const &rhs)
{
  return lhs.steamId != rhs.steamId;
}

json success(string cmd, json data)
{
  json res;
  res["success"] = true;
  res["cmd"] = cmd;
  res["data"] = data;
  return res;
}

json success(string cmd)
{
  return success(cmd, json::object());
}

json error(string msg)
{
  json res;
  res["success"] = false;
  res["error"] = msg;
  return res;
}

Server::Server(int maxPlayers) 
{
  this->rsa = generateRSA();
  this->maxPlayers = maxPlayers;
  pthread_mutex_init(&this->mutex, nullptr);
  this->game.inGame = false;
  this->game.versus = false;
  srand(time(NULL));
}

bool Server::handshake(Player* p)
{
  if (!ENCRYPT) return true;
  char buffer[BUFFER_SIZE];

  memset(buffer, 0, BUFFER_SIZE);
  strcpy(buffer, this->rsa.publicKey.c_str());
  send(p->fd, buffer, BUFFER_SIZE, 0);

  memset(buffer, 0, BUFFER_SIZE);
  ssize_t n = recv(p->fd, buffer, BUFFER_SIZE, 0);
  if (n <= 0) {
    return false;
  }

  string encryptedPayload(buffer);
  try {
    string decryptedPayload = decryptRSA(this->rsa.privateKey, encryptedPayload);
    json payload = json::parse(decryptedPayload);
    std::cout << payload << std::endl;
    if (payload["aesKey"].is_null() || payload["aesIV"].is_null()) {
      return false;
    }

    p->aesKey = payload["aesKey"];
    p->aesIV = payload["aesIV"];
    return true;
  } catch (const char* e) {
    if (DEBUG) std::cerr << e << std::endl;
    return false;
  }
}

void Server::join(Player p)
{
  if (this->canJoin(p)) {
    std::cout << "Player " << p.steamId << " joining" << std::endl;
    this->players.push_back(p);

    this->broadcast(success("JOIN", this->toJSON()));
  } else {
    this->sendToPlayer(p, error("Cannot join this server"));
  }
}

void Server::disconnect(Player p) {
  close(p.fd);
  if (!this->hasAlreadyJoined(p)) return;
  std::cout << "Player " << p.steamId << " disconnecting" << std::endl;
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

bool Server::hasAlreadyJoined(Player p)
{
  for (Player player : this->players) {
    if (player == p) return true;
  }
  return false;
}

bool Server::canJoin(Player p)
{
  if (this->isRunning()) return false;
  if (this->maxPlayers <= this->players.size()) return false;
  if (this->hasAlreadyJoined(p)) return false;
  return true;
}

void Server::sendToPlayer(Player receiver, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  string jsonString = ENCRYPT ? encryptAES(receiver.aesKey, receiver.aesIV, payload.dump()) : payload.dump();
  strcpy(buffer, jsonString.c_str());
  buffer[jsonString.size()] = '\n';
  if (DEBUG) std::cout << "[SENT - " << receiver.steamId << "] " << payload.dump() << std::endl;
  send(receiver.fd, buffer, BUFFER_SIZE, 0);
}

void Server::sendToOthers(Player sender, json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (Player player : playerList) {
    if (player != sender) this->sendToPlayer(player, payload);
  }
}

void Server::broadcast(json payload, bool ignoreEliminated)
{
  player_list_t playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (Player player : playerList) {
    this->sendToPlayer(player, payload);
  }
}

json Server::receive(Player sender) {
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  ssize_t n = recv(sender.fd, buffer, BUFFER_SIZE, 0);
  if (n <= 0) {
    this->lock();
    this->disconnect(sender);
    this->unlock();
    return json();
  }
  try {
    string decrypted = ENCRYPT ? decryptAES(sender.aesKey, sender.aesIV, string(buffer)) : string(buffer);
    if (DEBUG) std::cout << "[RECEIVED - " << sender.steamId << "] " << decrypted << std::endl;
    json req = json::parse(decrypted);
    return req;
  } catch (...) {
    this->lock();
    this->disconnect(sender);
    this->unlock();
    return json();
  }
}

bool Server::isHost(Player p) {
  return p == this->getHost();
}

Player Server::getHost() {
  return players.at(0);
}

void Server::start(Player sender, string seed, string deck, int stake, bool versus)
{
  if (this->isRunning() || !this->isHost(sender)) return;
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
  if (DEBUG) data["debug"] = true;
  this->broadcast(success("START", data));
  if (versus) this->broadcast(success("STATE_INFO", this->getState()));
}

void Server::stop()
{
  if (!this->isRunning()) return;
  std::cout << "Stopping multiplayer run" << std::endl;
  this->game.inGame = false;
}

bool Server::isRunning()
{
  return this->game.inGame;
}

bool Server::isVersus()
{
  return this->isRunning() && this->game.versus;
}

bool Server::isCoop()
{
  return this->isRunning() && !this->game.versus;
}

player_list_t Server::getRemainingPlayers() {
  if (!this->isVersus()) return this->players;
  player_list_t remaining;
  for (Player player : this->players) {
    bool eliminated = false;
    for (Player eliminatedPlayer : this->game.eliminated) {
      if (player == eliminatedPlayer) {
        eliminated = true;
        break;
      }
    }
    if (!eliminated) remaining.push_back(player);
  }
  return remaining;
}

player_list_t Server::getEliminatedPlayers() {
  return this->game.eliminated;
}

PersistentRequest *Server::createPersistentRequest(Player creator)
{
  int id = rand();
  PersistentRequest* req = new PersistentRequest;
  req->original = creator;
  req->data = json::object();
  req->id = std::to_string(id);
  req->created = clock();
  this->persistentRequests.push_back(req);
  return req;
}

PersistentRequest *Server::getPersistentRequest(string id)
{
  for (PersistentRequest* req : this->persistentRequests) {
    if (req->id == id) {
      if ((clock() - req->created)/CLOCKS_PER_SEC <= 10) {
        return req;
      } else {
        this->completePersistentRequest(id);
        return nullptr;
      }
    }
  }
  return nullptr;
}

void Server::completePersistentRequest(string id)
{
  for (int i = 0; i < this->persistentRequests.size(); i++) {
    PersistentRequest* req = this->persistentRequests[i];
    if (req->id == id) {
      this->persistentRequests.erase(this->persistentRequests.begin() + i);
      delete req;
    }
  }
}

void Server::endless()
{
  if (!this->isCoop()) return;
  this->broadcast(success("ENDLESS"));
}

void Server::highlight(Player sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

void Server::unhighlight(Player sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("UNHIGHLIGHT", data));
}

void Server::unhighlightAll(Player sender)
{
  if (!this->isCoop()) return;
  this->sendToOthers(sender, success("UNHIGHLIGHT_ALL"));
}

void Server::playHand(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("PLAY_HAND"));
}

void Server::discardHand(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("DISCARD_HAND"));
}

void Server::sortHand(Player sender, string sortType)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

void Server::reorder(Player sender, string selectType, int from, int to)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["from"] = from;
  data["to"] = to;
  this->sendToOthers(sender, success("REORDER", data));
}

void Server::selectBlind(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SELECT_BLIND"));
}

void Server::skipBlind(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BLIND"));
}

void Server::sell(Player sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("SELL", data));
}

void Server::use(Player sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("USE", data));
}

void Server::buy(Player sender, string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("BUY", data));
}

void Server::buyAndUse(Player sender, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("BUY_AND_USE", data));
}

void Server::skipBooster(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("SKIP_BOOSTER"));
}

void Server::reroll(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("REROLL"));
}

void Server::nextRound(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("NEXT_ROUND"));
}

void Server::goToShop(Player sender)
{
  if (!this->isCoop()) return;
  this->broadcast(success("GO_TO_SHOP"));
}

void Server::swapJokers(Player sender, json jokers)
{
  if (!this->isVersus()) return;
  player_list_t remainingPlayers = this->getRemainingPlayers();
  for (int i = 0; i < remainingPlayers.size(); i++) {
    if (remainingPlayers[i] == sender) {
      remainingPlayers.erase(remainingPlayers.begin() + i);
      break;
    }
  }
  if (remainingPlayers.size() == 0) return;
  int randomIndex = rand() % remainingPlayers.size();
  Player randomPlayer = remainingPlayers[randomIndex];
  PersistentRequest* preq = this->createPersistentRequest(sender);
  json data;
  data["jokers"] = jokers;
  data["request_id"] = preq->id;
  this->sendToPlayer(randomPlayer, success("SWAP_JOKERS", data));
}

void Server::swapJokers(Player sender, json jokers, string requestId)
{
  if (!this->isVersus()) return;
  json data;
  data["jokers"] = jokers;
  PersistentRequest* preq = this->getPersistentRequest(requestId);
  if (!preq) return;
  this->sendToPlayer(preq->original, success("SWAP_JOKERS", data));
  this->completePersistentRequest(preq->id);
}

void Server::changeMoney(Player sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToPlayer(sender, success("MONEY", data));
}

void Server::changeOthersMoney(Player sender, int change)
{
  if (!this->isVersus()) return;
  json data;
  data["money"] = change;
  this->sendToOthers(sender, success("MONEY", data));
}

void Server::changeHandSize(Player sender, int change, bool chooseRandom)
{
  if (!this->isVersus()) return;
  json data;
  data["hand_size"] = change;
  if (chooseRandom) {
    player_list_t remainingPlayers = this->getRemainingPlayers();
    for (int i = 0; i < remainingPlayers.size(); i++) {
      if (remainingPlayers[i] == sender) {
        remainingPlayers.erase(remainingPlayers.begin() + i);
        break;
      }
    }
    if (remainingPlayers.size() == 0) return;
    int randomIndex = rand() % remainingPlayers.size();
    Player randomPlayer = remainingPlayers[randomIndex];
    this->sendToPlayer(randomPlayer, success("HAND_SIZE", data));
  } else {
    this->sendToOthers(sender, success("HAND_SIZE", data), true);
  }
}

void Server::getCardsAndJokers(Player sender)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->createPersistentRequest(sender);
  preq->data["contributed"] = json::object();
  preq->data["contributed"][uint64ToString(sender.steamId)] = true;
  preq->data["results"] = json::object();
  preq->data["results"]["jokers"] = json::array();
  preq->data["results"]["cards"] = json::array();
  json data;
  data["request_id"] = preq->id;
  this->sendToOthers(sender, success("GET_CARDS_AND_JOKERS", data), true);
  for (Player p : this->getRemainingPlayers()) {
    if (!preq->data["contributed"][uint64ToString(p.steamId)].get<bool>()) return;
  }
  this->sendToPlayer(preq->original, success("GET_CARDS_AND_JOKERS", preq->data["results"]));
  this->completePersistentRequest(preq->id);
}

void Server::getCardsAndJokers(Player sender, json jokers, json cards, string requestId)
{
  if (!this->isVersus()) return;
  PersistentRequest* preq = this->getPersistentRequest(requestId);
  if (!preq) return;
  if (preq->data["contributed"][uint64ToString(sender.steamId)].get<bool>()) return;
  preq->data["contributed"][uint64ToString(sender.steamId)] = true;
  for (json joker : jokers) {
    preq->data["results"]["jokers"].push_back(joker);
  }
  for (json card : cards) {
    preq->data["results"]["cards"].push_back(card);
  }
  for (Player p : this->getRemainingPlayers()) {
    if (!preq->data["contributed"][uint64ToString(p.steamId)].get<bool>()) return;
  }
  this->sendToPlayer(preq->original, success("GET_CARDS_AND_JOKERS", preq->data["results"]));
  this->completePersistentRequest(requestId);
}

void Server::readyForBoss(Player sender)
{
  if (!this->isVersus()) return;
  if (this->game.bossPhase) return;
  for (Player player : this->game.ready) {
    if (player == sender) return;
  }
  this->game.ready.push_back(sender);
  for (Player player : this->getRemainingPlayers()) {
    bool isReady = false;
    for (Player readyPlayer : this->game.ready) {
      if (player == readyPlayer) {
        isReady = true;
        break;
      }
    }
    if (!isReady) return;
  }
  this->game.bossPhase = true;
  this->game.ready = player_list_t();
  this->game.scores = leaderboard_t();
  this->broadcast(success("START_BOSS"));
}

void Server::eliminate(Player p)
{
  if (!this->isVersus()) return;
  for (Player player : this->game.eliminated) {
    if (player == p) return;
  }
  this->game.eliminated.push_back(p);
  for (player_score_t pair : this->game.scores) {
    if (pair.first == p) {
      this->game.scores.erase(std::remove(this->game.scores.begin(), this->game.scores.end(), pair), this->game.scores.end());
      break;
    }
  }
  player_list_t remainingPlayers = this->getRemainingPlayers();
  if (remainingPlayers.size() == 1) {
    Player winner = remainingPlayers[0];
    this->sendToPlayer(winner, success("WIN"));
  } else {
    this->broadcast(success("STATE_INFO", this->getState()));
  }
}

void Server::defeatedBoss(Player p, double score)
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
      row["player"] = uint64ToString(pair.first.steamId);
      row["score"] = pair.second;
      data["leaderboard"].push_back(row);
    }
    player_list_t eliminatedPlayers = this->getEliminatedPlayers();
    std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
    for (Player player : eliminatedPlayers) {
      json row;
      row["player"] = uint64ToString(player.steamId);
      data["leaderboard"].push_back(row);
    }
    this->game.bossPhase = false;
    this->broadcast(success("LEADERBOARD", data));
  }
}

json Server::getState() {
  json state;
  state["remaining"] = this->getRemainingPlayers().size();
  return state;
}

json Server::toJSON() {
  json server;
  server["maxPlayers"] = this->maxPlayers;
  server["inGame"] = this->isRunning();

  json playerIds = json::array();
  for (Player player : this->players) {
    playerIds.push_back(uint64ToString(player.steamId));
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
