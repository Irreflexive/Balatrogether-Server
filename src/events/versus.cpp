#include "util/validation.hpp"
#include "util/response.hpp"
#include "events/versus.hpp"
#include "lobby.hpp"
#include "server.hpp"
#include "player.hpp"
#include "client.hpp"

using namespace balatrogether;

void SwapJokersEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (!req["jokers"].is_array()) throw std::invalid_argument("No jokers provided");

  if (req["request_id"].is_string()) {
    for (json joker : req["jokers"]) {
      if (!validation::string(joker["k"], 1, 32)) throw std::invalid_argument("No joker key");
    }

    string requestId = req["request_id"].get<string>();
    preq_t preq = lobby->getServer()->getPersistentRequestManager()->getById(requestId);
    if (!preq) return;

    json data;
    data["jokers"] = req["jokers"];
    for (client_t client : lobby->getClients()) {
      if (client->getPlayer()->getSteamId() == preq->getCreator()) {
        lobby->sendToPlayer(client, response::success("SWAP_JOKERS", data));
        break;
      }
    }
    lobby->getServer()->getPersistentRequestManager()->complete(requestId);
  } else {
    for (json joker : req["jokers"]) {
      if (!validation::string(joker["k"], 1, 32)) throw std::invalid_argument("No joker key");
    }

    preq_t preq = lobby->getServer()->getPersistentRequestManager()->create(client->getPlayer()->getSteamId());
    json data;
    data["jokers"] = req["jokers"];
    data["request_id"] = preq->getId();
    player_t randomPlayer = lobby->getGame()->getRandomPlayer(client->getPlayer());
    if (randomPlayer) lobby->sendToPlayer(randomPlayer->getClient(), response::success("SWAP_JOKERS", data));
  }
}

void GreenSealEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["money"] = -1;
  lobby->sendToOthers(client, response::success("MONEY", data));
}

void EraserEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  player_t randomPlayer = lobby->getGame()->getRandomPlayer(client->getPlayer());
  if (randomPlayer) lobby->sendToPlayer(randomPlayer->getClient(), response::success("HAND_SIZE", data));
}

void PaintBucketEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  lobby->sendToOthers(client, response::success("HAND_SIZE", data));
}

void GetCardsAndJokersEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  if (req["request_id"].is_string()) {
    if (!req["jokers"].is_array()) throw std::invalid_argument("No jokers provided");
    if (!req["cards"].is_array()) throw std::invalid_argument("No cards provided");

    string requestId = req["request_id"].get<string>();
    preq_t preq = lobby->getServer()->getPersistentRequestManager()->getById(requestId);
    if (!preq) return;

    json preqData = preq->getData();
    if (preqData["contributed"][client->getPlayer()->getSteamId()].get<bool>()) return;
    for (json joker : req["jokers"]) {
      if (!validation::string(joker["k"], 1, 32)) throw std::invalid_argument("No joker key");
      preqData["results"]["jokers"].push_back(joker);
    }
    for (json card : req["cards"]) {
      if (!validation::string(card["k"], 1, 32)) throw std::invalid_argument("No card key");
      preqData["results"]["cards"].push_back(card);
    }
    preqData["contributed"][client->getPlayer()->getSteamId()] = true;
    preq->setData(preqData);

    for (player_t p : lobby->getGame()->getRemaining()) {
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
    for (client_t client : lobby->getClients()) {
      if (client->getPlayer()->getSteamId() == preq->getCreator()) {
        lobby->sendToPlayer(client, response::success("GET_CARDS_AND_JOKERS", preqData["results"]));
        break;
      }
    }
    lobby->getServer()->getPersistentRequestManager()->complete(preq->getId());
  } else {
    preq_t preq = lobby->getServer()->getPersistentRequestManager()->create(client->getPlayer()->getSteamId());
    json preqData;
    preqData["contributed"] = json::object();
    preqData["contributed"][client->getPlayer()->getSteamId()] = true;
    preqData["results"] = json::object();
    preqData["results"]["jokers"] = json::array();
    preqData["results"]["cards"] = json::array();
    preq->setData(preqData);

    json data;
    data["request_id"] = preq->getId();
    lobby->sendToOthers(client, response::success("GET_CARDS_AND_JOKERS", data), true);
    for (player_t p : lobby->getGame()->getRemaining()) {
      if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
    }
    lobby->sendToPlayer(client, response::success("GET_CARDS_AND_JOKERS", preqData["results"]));
    lobby->getServer()->getPersistentRequestManager()->complete(preq->getId());
  }
}

void ReadyForBossEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  lobby->getGame()->prepareForBoss(client->getPlayer());
  if (lobby->getGame()->isBossReady()) {
    lobby->getGame()->setState(FIGHTING_BOSS);
    lobby->broadcast(response::success("START_BOSS"), true);
  }
}

void EliminatedEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  lobby->getGame()->eliminate(client->getPlayer());
  if (lobby->getGame()->getRemaining().size() == 1) {
    client_t winner = lobby->getClients().at(0);
    lobby->getGame()->reset();
    lobby->sendToPlayer(winner, response::success("WIN"));
  } else {
    lobby->broadcast(response::success("NODATA"));
  }
}

void DefeatedBossEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (validation::decimal(req["score"], 0)) throw std::runtime_error("No score provided");

  lobby->getGame()->addScore(client->getPlayer(), req["score"].get<double>());
  if (lobby->getGame()->isScoringFinished()) {
    json data;
    data["leaderboard"] = lobby->getGame()->getLeaderboard();
    lobby->getGame()->setState(IN_PROGRESS);
    lobby->broadcast(response::success("LEADERBOARD", data), true);
  }
}
