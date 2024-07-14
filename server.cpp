#include "server.hpp"

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
  this->inGame = false;
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
  if (this->inGame) return false;
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

void Server::start(Player sender, std::string seed, std::string deck, int stake)
{
  if (this->inGame || !this->isHost(sender)) return;
  std::cout << "Starting multiplayer run" << std::endl;
  this->inGame = true;
  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  this->broadcast(success("START", data));
}

void Server::stop()
{
  if (!this->inGame) return;
  std::cout << "Stopping multiplayer run" << std::endl;
  this->inGame = false;
}

void Server::endless()
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("ENDLESS", data));
}

void Server::highlight(Player sender, std::string selectType, int index)
{
  if (!this->inGame) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("HIGHLIGHT", data));
}

void Server::unhighlight(Player sender, std::string selectType, int index)
{
  if (!this->inGame) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->sendToOthers(sender, success("UNHIGHLIGHT", data));
}

void Server::unhighlightAll(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->sendToOthers(sender, success("UNHIGHLIGHT_ALL", data));
}

void Server::playHand(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("PLAY_HAND", data));
}

void Server::discardHand(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("DISCARD_HAND", data));
}

void Server::sortHand(Player sender, std::string sortType)
{
  if (!this->inGame) return;
  json data;
  data["type"] = sortType;
  this->broadcast(success("SORT_HAND", data));
}

void Server::reorder(Player sender, std::string selectType, int from, int to)
{
  if (!this->inGame) return;
  json data;
  data["type"] = selectType;
  data["from"] = from;
  data["to"] = to;
  this->sendToOthers(sender, success("REORDER", data));
}

void Server::selectBlind(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("SELECT_BLIND", data));
}

void Server::skipBlind(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("SKIP_BLIND", data));
}

void Server::sell(Player sender, std::string selectType, int index)
{
  if (!this->inGame) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("SELL", data));
}

void Server::use(Player sender, int index)
{
  if (!this->inGame) return;
  json data;
  data["index"] = index;
  this->broadcast(success("USE", data));
}

void Server::buy(Player sender, std::string selectType, int index)
{
  if (!this->inGame) return;
  json data;
  data["type"] = selectType;
  data["index"] = index;
  this->broadcast(success("BUY", data));
}

void Server::buyAndUse(Player sender, int index)
{
  if (!this->inGame) return;
  json data;
  data["index"] = index;
  this->broadcast(success("BUY_AND_USE", data));
}

void Server::skipBooster(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("SKIP_BOOSTER", data));
}

void Server::reroll(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("REROLL", data));
}

void Server::nextRound(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("NEXT_ROUND", data));
}

void Server::goToShop(Player sender)
{
  if (!this->inGame) return;
  json data = json::object();
  this->broadcast(success("GO_TO_SHOP", data));
}

json Server::toJSON() {
  json server;
  server["inGame"] = this->inGame;

  json playerIds = json::array();
  for (Player player : this->players) {
    std::ostringstream os;
    os << player.steamId;
    playerIds.push_back(os.str());
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
