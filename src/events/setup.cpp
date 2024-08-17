#include "util/logs.hpp"
#include "util/response.hpp"
#include "util/validation.hpp"
#include "events/setup.hpp"
#include "server.hpp"
#include "lobby.hpp"
#include "client.hpp"

using namespace balatrogether;

void connectToServer(server_t server, client_t client, json req) {
  if (!validation::steamid(req["steam_id"])) throw std::invalid_argument("No Steam ID provided");
  if (!validation::string(req["unlock_hash"], 64, 128) || !validation::base64(req["unlock_hash"])) throw std::invalid_argument("No unlock hash provided");
  if (!req["stakes"].is_object() || req["stakes"].size() > 20) throw std::invalid_argument("No stake unlocks provided");

  player_auth auth;
  auth.steamId = req["steam_id"].get<steamid_t>();
  auth.unlockHash = req["unlock_hash"].get<string>();
  auth.stakes = req["stakes"].get<std::unordered_map<string, int>>();
  if (server->getConfig()->isSteamApiEnabled()) auth.fetchKey = server->getConfig()->getSteamApiKey();
  server->connect(client, auth);
}

void JoinEvent::execute(server_t server, client_t client, json req)
{
  connectToServer(server, client, req);
  lobby_t defaultLobby = server->getDefaultLobby();
  if (defaultLobby) {
    defaultLobby->add(client);
  } else {
    json lobbies = json::array();
    for (lobby_t lobby : server->getLobbies()) {
      json status;
      status["open"] = lobby->canJoin(client);
      status["players"] = lobby->getClients().size();
      status["max"] = server->getConfig()->getMaxPlayers();
      lobbies.push_back(status);
    }
    server->getNetworkManager()->send({client}, response::success("LOBBIES", lobbies));
    server->disconnect(client);
  }
}

void JoinLobbyEvent::execute(server_t server, client_t client, json req)
{
  if (server->getDefaultLobby()) throw std::runtime_error("Bad request");
  if (!validation::integer(req["number"], 1, server->getConfig()->getMaxLobbies())) throw std::invalid_argument("No lobby number provided");
  lobby_t lobby = server->getLobby(req["number"].get<int>());
  if (!lobby) throw std::invalid_argument("No lobby found");

  connectToServer(server, client, req);
  lobby->add(client);
}

void StartRunEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!validation::string(req["seed"], 8, 8)) throw std::invalid_argument("No seed provided");
  if (!validation::string(req["deck"], 1, 32)) throw std::invalid_argument("No deck provided");
  if (!validation::integer(req["stake"], 1, 8)) throw std::invalid_argument("No stake provided");
  if (!validation::boolean(req["versus"])) throw std::invalid_argument("No game mode provided");
  if (!req["versus"].get<bool>() && !validation::integer(req["speed"], 0, 32)) throw std::invalid_argument("Invalid game speed");
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
        throw client_exception("Players do not have the same unlocks");
      }
    }
  }
  for (player_t p : players) {
    if (p->getUnlockedStake(deck) < stake) throw client_exception("Players do not have deck/stake unlocked");
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

  logger::stream info = lobby->getLogger();
  info << "===========START RUN===========" << std::endl;
  info << std::left << std::setw(10) << "MODE" << std::right << std::setw(21) << (versus ? "VERSUS" : "CO-OP") << std::endl;
  info << std::left << std::setw(10) << "SEED" << std::right << std::setw(21) << seed << std::endl;
  string upperDeck = deck;
  std::transform(upperDeck.begin(), upperDeck.end(), upperDeck.begin(), [](unsigned char c){ return std::toupper(c); });
  info << std::left << std::setw(10) << "DECK" << std::right << std::setw(21) << upperDeck << std::endl;
  info << std::left << std::setw(10) << "STAKE" << std::right << std::setw(21) << ((stake >= 1 && stake <= 8) ? stakes[stake - 1] : "UNKNOWN") << std::endl;
  info << "===============================" << std::endl;

  lobby->getGame()->start(players, versus);

  json data;
  data["seed"] = seed;
  data["deck"] = deck;
  data["stake"] = stake;
  data["versus"] = versus;
  data["speed"] = req["speed"].get<int>();
  if (validation::boolean(req["debug"]) && req["debug"].get<bool>() && lobby->getServer()->getConfig()->isDebugMode())
    data["debug"] = true;
  lobby->broadcast(response::success("START", data));
}
