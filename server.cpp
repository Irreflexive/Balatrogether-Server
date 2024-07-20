#include "server.hpp"

std::string uint64ToString(uint64_t n) {
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

json success(std::string cmd, json data)
{
  json res;
  res["success"] = true;
  res["cmd"] = cmd;
  res["data"] = data;
  return res;
}

json success(std::string cmd)
{
  return success(cmd, json::object());
}

json error(std::string msg)
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

  std::string encryptedPayload(buffer);
  try {
    std::string decryptedPayload = decryptRSA(this->rsa.privateKey, encryptedPayload);
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
  std::string jsonString = ENCRYPT ? encryptAES(receiver.aesKey, receiver.aesIV, payload.dump()) : payload.dump();
  strcpy(buffer, jsonString.c_str());
  buffer[jsonString.size()] = '\n';
  if (DEBUG) std::cout << "[SENT - " << receiver.steamId << "] " << payload.dump() << std::endl;
  send(receiver.fd, buffer, BUFFER_SIZE, 0);
}

void Server::sendToOthers(Player sender, json payload, bool ignoreEliminated)
{
  std::vector<Player> playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
  for (Player player : playerList) {
    if (player != sender) this->sendToPlayer(player, payload);
  }
}

void Server::broadcast(json payload, bool ignoreEliminated)
{
  std::vector<Player> playerList = ignoreEliminated ? this->getRemainingPlayers() : this->players;
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
    std::string decrypted = ENCRYPT ? decryptAES(sender.aesKey, sender.aesIV, std::string(buffer)) : std::string(buffer);
    if (DEBUG) std::cout << "[RECEIVED] " << decrypted << std::endl;
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

void Server::start(Player sender, std::string seed, std::string deck, int stake, bool versus)
{
  if (this->isRunning() || !this->isHost(sender)) return;
  std::cout << "Starting multiplayer run" << std::endl;

  this->game.inGame = true;
  this->game.versus = versus;
  this->game.bossPhase = false;
  this->game.eliminated = std::vector<Player>();
  this->game.ready = std::vector<Player>();
  this->game.scores = std::vector<player_score_t>();
  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  this->broadcast(success("START", data));
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

std::vector<Player> Server::getRemainingPlayers() {
  if (!this->isVersus()) return this->players;
  std::vector<Player> remaining;
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

std::vector<Player> Server::getEliminatedPlayers() {
  return this->game.eliminated;
}

void Server::endless()
{
  if (!this->isCoop()) return;
  this->broadcast(success("ENDLESS"));
}

void Server::highlight(Player sender, std::string selectType, int index)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

void Server::unhighlight(Player sender, std::string selectType, int index)
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

void Server::sortHand(Player sender, std::string sortType)
{
  if (!this->isCoop()) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

void Server::reorder(Player sender, std::string selectType, int from, int to)
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

void Server::sell(Player sender, std::string selectType, int index)
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

void Server::buy(Player sender, std::string selectType, int index)
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

void Server::annieAndHallie(Player sender, json jokers)
{
  if (!this->isVersus()) return;
  std::vector<Player> remainingPlayers = this->getRemainingPlayers();
  for (int i = 0; i < remainingPlayers.size(); i++) {
    if (remainingPlayers[i] == sender) {
      remainingPlayers.erase(remainingPlayers.begin() + i);
      break;
    }
  }
  if (remainingPlayers.size() == 0) return;
  int randomIndex = rand() % remainingPlayers.size();
  Player randomPlayer = remainingPlayers[randomIndex];
  json data;
  data["jokers"] = jokers;
  data["user"] = uint64ToString(sender.steamId);
  this->sendToPlayer(randomPlayer, success("ANNIE_AND_HALLIE", data));
}

void Server::annieAndHallie(Player sender, json jokers, std::string targetId)
{
  if (!this->isVersus()) return;
  json data;
  data["jokers"] = jokers;
  for (Player player : this->players) {
    if (uint64ToString(player.steamId) == targetId) {
      this->sendToPlayer(player, success("ANNIE_AND_HALLIE", data));
      break;
    }
  }
}

void Server::theCup(Player sender)
{
  if (!this->isVersus()) return;
  json data;
  data["eliminated"] = this->getEliminatedPlayers().size();
  this->broadcast(success("THE_CUP", data));
}

void Server::greenSeal(Player sender)
{
  if (!this->isVersus()) return;
  this->sendToOthers(sender, success("GREEN_SEAL"));
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
  this->game.ready = std::vector<Player>();
  this->game.scores = std::vector<player_score_t>();
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
  std::vector<Player> remainingPlayers = this->getRemainingPlayers();
  if (remainingPlayers.size() == 1) {
    Player winner = remainingPlayers[0];
    this->sendToPlayer(winner, success("WIN"));
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
    std::vector<Player> eliminatedPlayers = this->getEliminatedPlayers();
    std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
    for (Player player : eliminatedPlayers) {
      json row;
      row["player"] = uint64ToString(player.steamId);
    }
    this->game.bossPhase = false;
    this->broadcast(success("LEADERBOARD", data));
  }
}

json Server::toJSON() {
  json server;
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
