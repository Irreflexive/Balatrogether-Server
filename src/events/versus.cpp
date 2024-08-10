#include "util.hpp"
#include "events/versus.hpp"

void SwapJokersEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (!req["jokers"].is_array()) throw std::invalid_argument("No jokers provided");

  if (req["request_id"].is_string()) {
    for (json joker : req["jokers"]) {
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
    }

    string requestId = req["request_id"].get<string>();
    PersistentRequest* preq = server->getPersistentRequestManager()->getById(requestId);
    if (!preq) return;

    json data;
    data["jokers"] = req["jokers"];
    server->sendToPlayer(preq->getCreator()->getClient(), success("SWAP_JOKERS", data));
    server->getPersistentRequestManager()->complete(requestId);
  } else {
    for (json joker : req["jokers"]) {
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
    }

    PersistentRequest* preq = server->getPersistentRequestManager()->create(client->getPlayer());
    json data;
    data["jokers"] = req["jokers"];
    data["request_id"] = preq->getId();
    server->sendToRandom(client, success("SWAP_JOKERS", data));
  }
}

void TheCupEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["money"] = server->getGame()->getEliminated().size() * 8;
  server->sendToPlayer(client, success("MONEY", data));
}

void GreenSealEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["money"] = -1;
  server->sendToOthers(client, success("MONEY", data));
}

void EraserEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  server->sendToRandom(client, success("HAND_SIZE", data));
}

void PaintBucketEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  server->sendToOthers(client, success("HAND_SIZE", data));
}

void GetCardsAndJokersEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  if (req["request_id"].is_string()) {
    if (!req["jokers"].is_array()) throw std::invalid_argument("No jokers provided");
    if (!req["cards"].is_array()) throw std::invalid_argument("No cards provided");

    string requestId = req["request_id"].get<string>();
    PersistentRequest* preq = server->getPersistentRequestManager()->getById(requestId);
    if (!preq) return;

    json preqData = preq->getData();
    if (preqData["contributed"][client->getPlayer()->getSteamId()].get<bool>()) return;
    for (json joker : req["jokers"]) {
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
      preqData["results"]["jokers"].push_back(joker);
    }
    for (json card : req["cards"]) {
      if (card["k"].is_null()) throw std::invalid_argument("No card key");
      preqData["results"]["cards"].push_back(card);
    }
    preqData["contributed"][client->getPlayer()->getSteamId()] = true;
    preq->setData(preqData);

    for (player_t p : server->getGame()->getRemaining()) {
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
    server->sendToPlayer(preq->getCreator()->getClient(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
    server->getPersistentRequestManager()->complete(preq->getId());
  } else {
    PersistentRequest* preq = server->getPersistentRequestManager()->create(client->getPlayer());
    json preqData;
    preqData["contributed"] = json::object();
    preqData["contributed"][client->getPlayer()->getSteamId()] = true;
    preqData["results"] = json::object();
    preqData["results"]["jokers"] = json::array();
    preqData["results"]["cards"] = json::array();
    preq->setData(preqData);

    json data;
    data["request_id"] = preq->getId();
    server->sendToOthers(client, success("GET_CARDS_AND_JOKERS", data), true);
    for (player_t p : server->getGame()->getRemaining()) {
      if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
    }
    server->sendToPlayer(client, success("GET_CARDS_AND_JOKERS", preqData["results"]));
    server->getPersistentRequestManager()->complete(preq->getId());
  }
}

void ReadyForBossEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  server->getGame()->prepareForBoss(client->getPlayer());
  if (server->getGame()->isBossReady()) {
    server->getGame()->setBossPhaseEnabled(true);
    server->broadcast(success("START_BOSS"), true);
  }
}

void EliminatedEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  server->eliminate(client->getPlayer());
}

void DefeatedBossEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (!req["score"].is_number_float()) throw std::runtime_error("No score provided");

  server->getGame()->addScore(client->getPlayer(), req["score"].get<double>());
  if (server->getGame()->isScoringFinished()) {
    json data;
    data["leaderboard"] = server->getGame()->getLeaderboard();
    server->getGame()->setBossPhaseEnabled(false);
    server->broadcast(success("LEADERBOARD", data), true);
  }
}
