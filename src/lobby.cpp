#include "events/setup.hpp"
#include "events/coop.hpp"
#include "events/versus.hpp"
#include "lobby.hpp"

const char* roomCodeAlphabet = "0123456789ABCDEF";

Lobby::Lobby(server_t server) 
{
  for (int i = 0; i < ROOM_CODE_LEN; i++) {
    this->code += roomCodeAlphabet[rand() % strlen(roomCodeAlphabet)];
  }
  this->server = server;
  this->listener = new EventListener<lobby_t>(this);
  this->game = new Game;

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
  this->getServer()->getNetworkManager()->send({client}, payload);
}

void Lobby::sendToOthers(client_t client, json payload, bool ignoreEliminated)
{
  client_list_t clients;
  for (client_t c : this->getClients()) {
    if (client != c && (!ignoreEliminated || !this->getGame()->isEliminated(c->getPlayer()))) clients.push_back(c);
  }
  this->getServer()->getNetworkManager()->send(clients, payload);
}

void Lobby::broadcast(json payload, bool ignoreEliminated)
{
  client_list_t clients;
  for (client_t c : this->getClients()) {
    if (!ignoreEliminated || !this->getGame()->isEliminated(c->getPlayer())) clients.push_back(c);
  }
  this->getServer()->getNetworkManager()->send(clients, payload);
}

void Lobby::add(client_t client)
{
  if (!client->getPlayer()) return;
  this->clients.push_back(client);
  client->setLobby(this);
  logger::info("[%s] Player %s joined lobby", this->getCode().c_str(), client->getPlayer()->getSteamId().c_str());
  this->broadcast(success("JOIN", this->getJSON()));
}

void Lobby::remove(client_t client)
{
  logger::info("[%s] Player %s left lobby", this->getCode().c_str(), client->getPlayer()->getSteamId().c_str());
  this->getGame()->eliminate(client->getPlayer());
  for (int i = 0; i < this->clients.size(); i++) {
    client_t c = this->clients.at(i);
    if (c == client) {
      this->clients.erase(this->clients.begin() + i);
      client->setLobby(nullptr);
      this->broadcast(success("LEAVE", this->getJSON()));
      break;
    }
  }
  if (this->clients.size() == 0) this->getServer()->deleteLobby(this);
}

void Lobby::close()
{
  this->getGame()->reset();
  for (client_t c : this->getClients()) {
    this->getServer()->disconnect(c);
  }
}

string Lobby::getCode()
{
  return this->code;
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

json Lobby::getJSON()
{
  json data;
  data["players"] = json::array();
  data["maxPlayers"] = this->getServer()->getConfig()->getMaxPlayers();

  for (client_t c : this->getClients()) {
    data["players"].push_back(c->getPlayer()->getSteamId());
  }

  return data;
}
