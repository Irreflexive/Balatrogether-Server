#include "util.hpp"
#include "events/setup.hpp"

void JoinEvent::execute(server_t server, client_t client, json req)
{
  if (!req["steam_id"].is_string()) throw std::invalid_argument("No Steam ID provided");
  if (!req["unlock_hash"].is_string()) throw std::invalid_argument("No unlock provided");

  string steamId = req["steam_id"].get<string>();
  string unlockHash = req["unlock_hash"].get<string>();
  server->connect(client, steamId, unlockHash);
  if (server->getConfig()->getMaxLobbies() == 1) {
    lobby_t defaultLobby = server->getLobby();
    if (!defaultLobby) defaultLobby = server->createLobby();
    defaultLobby->add(client);
  } else {
    json data = server->getLobbyCodes();
    server->getNetworkManager()->send({client}, success("LOBBIES", data));
  }
}

void StartRunEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!req["seed"].is_string()) throw std::invalid_argument("No seed provided");
  if (!req["deck"].is_string()) throw std::invalid_argument("No deck provided");
  if (!req["stake"].is_number_integer()) throw std::invalid_argument("No stake provided");
  if (!req["versus"].is_boolean()) throw std::invalid_argument("No game mode provided");
  if (lobby->getGame()->isRunning() || !lobby->isHost(client) || (lobby->getClients().size() <= 1 && !lobby->getServer()->getConfig()->isDebugMode())) {
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

  logger::info("[%S] ===========START RUN===========", lobby->getCode().c_str());
  logger::info("[%S] %-10s %20s", lobby->getCode().c_str(), "MODE", versus ? "VERSUS" : "CO-OP");
  logger::info("[%S] %-10s %20s", lobby->getCode().c_str(), "SEED", seed.c_str());
  string upperDeck = deck;
  std::transform(upperDeck.begin(), upperDeck.end(), upperDeck.begin(), [](unsigned char c){ return std::toupper(c); });
  logger::info("[%S] %-10s %20s", lobby->getCode().c_str(), "DECK", upperDeck.c_str());
  logger::info("[%S] %-10s %20s", lobby->getCode().c_str(), "STAKE", (stake >= 1 && stake <= 8) ? stakes[stake - 1] : "UNKNOWN");

  lobby->getGame()->start(players, versus);

  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  data["eliminated"] = lobby->getGame()->getEliminated().size();
  if (lobby->getServer()->getConfig()->isDebugMode()) data["debug"] = true;
  lobby->broadcast(success("START", data));
}
