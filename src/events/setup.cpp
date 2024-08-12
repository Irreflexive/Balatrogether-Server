#include "util.hpp"
#include "events/setup.hpp"

void JoinEvent::execute(server_t server, client_t client, json req)
{
  if (!req["steam_id"].is_string()) throw std::invalid_argument("No Steam ID provided");
  if (!req["unlock_hash"].is_string()) throw std::invalid_argument("No unlock provided");

  string steamId = req["steam_id"].get<string>();
  string unlockHash = req["unlock_hash"].get<string>();
  server->connect(client, steamId, unlockHash);
  lobby_t defaultLobby = server->getDefaultLobby();
  if (defaultLobby) {
    defaultLobby->add(client);
  } else {
    // TODO: add json for each lobby
    server->getNetworkManager()->send({client}, success("LOBBIES"));
    server->disconnect(client);
  }
}

void JoinLobbyEvent::execute(server_t server, client_t client, json req)
{
  if (server->getDefaultLobby()) throw std::runtime_error("Cannot join by code");
  if (!req["number"].is_string()) throw std::invalid_argument("No lobby number provided");

  lobby_t lobby = server->getLobby(req["number"].get<int>());
  if (!lobby) throw std::invalid_argument("No lobby found");

  lobby->add(client);
}

void StartRunEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!req["seed"].is_string()) throw std::invalid_argument("No seed provided");
  if (!req["deck"].is_string()) throw std::invalid_argument("No deck provided");
  if (!req["stake"].is_number_integer()) throw std::invalid_argument("No stake provided");
  if (!req["versus"].is_boolean()) throw std::invalid_argument("No game mode provided");
  if (!lobby->isHost(client) || (lobby->getClients().size() <= 1 && !lobby->getServer()->getConfig()->isDebugMode())) {
    throw std::runtime_error("Cannot start run");
  }
  
  string seed = req["seed"].get<string>();
  string deck = req["deck"].get<string>();
  int stake = req["stake"].get<int>();
  bool versus = req["versus"].get<bool>();

  lobby->getGame()->reset();
  player_list_t players;
  for (client_t client : lobby->getClients()) {
    players.push_back(client->getPlayer());
  }

  if (!versus) {
    bool initialized = false;
    string unlockHash;
    for (player_t p : players) {
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

  logger::info("[ROOM %d] ===========START RUN===========", lobby->getRoomNumber());
  logger::info("[ROOM %d] %-10s %20s", lobby->getRoomNumber(), "MODE", versus ? "VERSUS" : "CO-OP");
  logger::info("[ROOM %d] %-10s %20s", lobby->getRoomNumber(), "SEED", seed.c_str());
  string upperDeck = deck;
  std::transform(upperDeck.begin(), upperDeck.end(), upperDeck.begin(), [](unsigned char c){ return std::toupper(c); });
  logger::info("[ROOM %d] %-10s %20s", lobby->getRoomNumber(), "DECK", upperDeck.c_str());
  logger::info("[ROOM %d] %-10s %20s", lobby->getRoomNumber(), "STAKE", (stake >= 1 && stake <= 8) ? stakes[stake - 1] : "UNKNOWN");
  logger::info("[ROOM %d] ===============================", lobby->getRoomNumber());

  lobby->getGame()->start(players, versus);

  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  data["state"] = lobby->getGame()->getJSON();
  if (lobby->getServer()->getConfig()->isDebugMode()) data["debug"] = true;
  lobby->broadcast(success("START", data));
}
