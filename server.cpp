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

Server::Server(int maxPlayers, bool debugMode) 
{
  this->maxPlayers = maxPlayers;
  this->debugMode = debugMode;
  pthread_mutex_init(&this->mutex, nullptr);
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
  this->game.eliminate(p);
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
  if (this->game.isRunning()) return false;
  if (this->maxPlayers <= this->players.size()) return false;
  if (this->hasAlreadyJoined(p)) return false;
  return true;
}

void Server::sendToPlayer(Player receiver, json payload)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  std::string jsonString = payload.dump();
  strcpy(buffer, jsonString.c_str());
  buffer[jsonString.size()] = '\n';
  if (debugMode) std::cout << "[SENT - " << receiver.steamId << "] " << buffer << std::endl;
  send(receiver.fd, buffer, BUFFER_SIZE, 0);
}

void Server::sendToOthers(Player sender, json payload)
{
  for (Player player : this->players) {
    if (player != sender) this->sendToPlayer(player, payload);
  }
}

void Server::broadcast(json payload)
{
  for (Player player : this->players) {
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
  if (debugMode) std::cout << "[RECEIVED] " << buffer << std::endl;
  try {
    json req = json::parse(buffer);
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
  if (this->game.isRunning() || !this->isHost(sender)) return;
  std::cout << "Starting multiplayer run" << std::endl;

  this->game.start(versus);
  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  this->broadcast(success("START", data));
}

void Server::stop()
{
  if (!this->game.isRunning()) return;
  std::cout << "Stopping multiplayer run" << std::endl;
  this->game.stop();
}

void Server::endless()
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("ENDLESS"));
}

void Server::highlight(Player sender, std::string selectType, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

void Server::unhighlight(Player sender, std::string selectType, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("UNHIGHLIGHT", data));
}

void Server::unhighlightAll(Player sender)
{
  if (!this->game.isCoop()) return;
  this->sendToOthers(sender, success("UNHIGHLIGHT_ALL"));
}

void Server::playHand(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("PLAY_HAND"));
}

void Server::discardHand(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("DISCARD_HAND"));
}

void Server::sortHand(Player sender, std::string sortType)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

void Server::reorder(Player sender, std::string selectType, int from, int to)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = selectType;
  data["from"] = from;
  data["to"] = to;
  this->sendToOthers(sender, success("REORDER", data));
}

void Server::selectBlind(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("SELECT_BLIND"));
}

void Server::skipBlind(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("SKIP_BLIND"));
}

void Server::sell(Player sender, std::string selectType, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("SELL", data));
}

void Server::use(Player sender, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("USE", data));
}

void Server::buy(Player sender, std::string selectType, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("BUY", data));
}

void Server::buyAndUse(Player sender, int index)
{
  if (!this->game.isCoop()) return;
  json data;
  data["index"] = index;
  this->broadcast(success("BUY_AND_USE", data));
}

void Server::skipBooster(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("SKIP_BOOSTER"));
}

void Server::reroll(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("REROLL"));
}

void Server::nextRound(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("NEXT_ROUND"));
}

void Server::goToShop(Player sender)
{
  if (!this->game.isCoop()) return;
  this->broadcast(success("GO_TO_SHOP"));
}

void Server::annieAndHallie(Player sender, json jokers, bool responding, json targetResponse)
{
  if (!this->game.isVersus()) return;
  if (!responding) { // no req["player"], so it is the initial sender
    std::vector<Player> remainingPlayers = this->game.getRemainingPlayers(this->players);
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
  } else { // target sent a response with the jokers they are giving up
    std::string targetId = targetResponse.template get<std::string>();
    json data;
    data["jokers"] = jokers;
    for (Player player : this->players) {
      if (uint64ToString(player.steamId) == targetId) {
        this->sendToPlayer(player, success("ANNIE_AND_HALLIE", data));
        break;
      }
    }
  }
}

void Server::theCup(Player sender)
{
  if (!this->game.isVersus()) return;
  json data;
  data["eliminated"] = this->game.getEliminatedPlayers().size();
  this->broadcast(success("THE_CUP", data));
}

void Server::readyForBoss(Player sender)
{
  if (!this->game.isVersus()) return;
  this->game.markReadyForBoss(sender);
  if (this->game.ready.size() == this->game.getRemainingPlayers(this->players).size()) {
    this->game.startBoss();
    this->broadcast(success("START_BOSS"));
  }
}

json Server::toJSON() {
  json server;
  server["inGame"] = this->game.isRunning();

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

Game::Game()
{
  this->inGame = false;
  this->versus = false;
}

void Game::start(bool versus)
{
  this->inGame = true;
  this->versus = versus;
  this->eliminated = std::vector<Player>();
}

void Game::stop()
{
  this->inGame = false;
}

bool Game::isRunning()
{
  return this->inGame;
}

bool Game::isVersus()
{
  return this->isRunning() && this->versus;
}

bool Game::isCoop()
{
  return this->isRunning() && !this->versus;
}

std::vector<Player> Game::getRemainingPlayers(std::vector<Player> connected)
{
  std::vector<Player> remaining;
  for (Player player : connected) {
    bool eliminated = false;
    for (Player eliminatedPlayer : this->eliminated) {
      if (player == eliminatedPlayer) {
        eliminated = true;
        break;
      }
    }
    if (!eliminated) remaining.push_back(player);
  }
  return remaining;
}

std::vector<Player> Game::getEliminatedPlayers()
{
  return this->eliminated;
}

void Game::eliminate(Player p)
{
  for (Player player : this->eliminated) {
    if (player == p) return;
  }
  this->eliminated.push_back(p);
}

void Game::markReadyForBoss(Player p)
{
  for (Player player : this->ready) {
    if (player == p) return;
  }
  this->ready.push_back(p);
}

void Game::startBoss()
{
  this->ready = std::vector<Player>(); 
}
