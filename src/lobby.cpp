#include "util/response.hpp"
#include "lobby.hpp"
#include "server.hpp"
#include "client.hpp"

using namespace balatrogether;

Lobby::Lobby(server_t server, int roomNumber) : logger(string("[INFO] [ROOM ") + std::to_string(roomNumber) + "] ", std::cout)
{
  this->roomNumber = roomNumber;
  this->server = server;
  this->listener = new LobbyEventListener(this);
  this->game = new Game;
  this->getLogger() << "Lobby initialization complete" << std::endl;
}

Lobby::~Lobby() 
{
  delete this->listener;
  delete this->game;
}

bool Lobby::canJoin(client_t client)
{
  if (this->getGame()->isRunning()) return false;
  if (this->getServer()->getConfig()->getMaxPlayers() <= this->getClients().size()) return false;
  for (client_t c : this->getClients()) {
    if (client->getIdentity() == c->getIdentity()) return false;
  }
  return true;
}

bool Lobby::isHost(client_t client)
{
  return client == this->getHost();
}

client_t Lobby::getHost()
{
  return this->getClients().size() > 0 ? this->getClients().at(0) : nullptr;
}

void Lobby::sendToPlayer(client_t client, json payload)
{
  payload["game_state"] = this->getGame()->getJSON();
  this->getServer()->getNetworkManager()->send({client}, payload);
}

void Lobby::sendToOthers(client_t client, json payload, bool ignoreEliminated)
{
  payload["game_state"] = this->getGame()->getJSON();
  client_list_t clients;
  for (client_t c : this->getClients()) {
    if (client != c && (!ignoreEliminated || !this->getGame()->isEliminated(c->getPlayer()))) clients.push_back(c);
  }
  this->getServer()->getNetworkManager()->send(clients, payload);
}

void Lobby::broadcast(json payload, bool ignoreEliminated)
{
  payload["game_state"] = this->getGame()->getJSON();
  client_list_t clients;
  for (client_t c : this->getClients()) {
    if (!ignoreEliminated || !this->getGame()->isEliminated(c->getPlayer())) clients.push_back(c);
  }
  this->getServer()->getNetworkManager()->send(clients, payload);
}

void Lobby::add(client_t client)
{
  if (!client->getPlayer()) return;
  if (!this->canJoin(client)) throw client_exception("Cannot join lobby");
  this->clients.push_back(client);
  client->setLobby(this);
  this->getLogger() << "Player " << client->getPlayer()->getSteamId() << " joined lobby" << std::endl;
  this->broadcast(response::success("JOIN", this->getJSON()));
}

void Lobby::remove(client_t client)
{
  this->getLogger() << "Player " << client->getPlayer()->getSteamId() << " left lobby" << std::endl;
  this->getGame()->eliminate(client->getPlayer());
  for (int i = 0; i < this->clients.size(); i++) {
    client_t c = this->clients.at(i);
    if (c == client) {
      this->clients.erase(this->clients.begin() + i);
      client->setLobby(nullptr);
      this->broadcast(response::success("LEAVE", this->getJSON()));
      break;
    }
  }
  if (this->getClients().size() == 0) this->close();
}

void Lobby::close()
{
  this->getGame()->reset();
  for (client_t c : this->getClients()) {
    this->getServer()->disconnect(c);
  }
}

int Lobby::getRoomNumber()
{
  return this->roomNumber;
}

server_t Lobby::getServer()
{
  return this->server;
}

lobby_listener_t Lobby::getEventListener()
{
  return this->listener;
}

client_list_t Lobby::getClients()
{
  return this->clients;
}

game_t Lobby::getGame()
{
  return this->game;
}

logger::stream Lobby::getLogger()
{
  return this->logger;
}

json Lobby::getJSON()
{
  json data;
  data["players"] = json::array();
  for (client_t c : this->getClients()) {
    json p;
    p["id"] = c->getPlayer()->getSteamId();
    if (this->getServer()->getConfig()->isSteamApiEnabled()) p["name"] = c->getPlayer()->getName();
    data["players"].push_back(p);
  }
  data["maxPlayers"] = this->getServer()->getConfig()->getMaxPlayers();
  if (this != this->getServer()->getDefaultLobby()) data["room"] = this->getRoomNumber();
  return data;
}
