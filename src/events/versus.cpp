#include "util.hpp"
#include "events/versus.hpp"

void SwapJokersEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (!req["jokers"].is_array()) throw std::invalid_argument("No jokers provided");

  if (req["request_id"].is_string()) {
    for (json joker : req["jokers"]) {
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
    }

    string requestId = req["request_id"].get<string>();
    preq_t preq = lobby->getServer()->getPersistentRequestManager()->getById(requestId);
    if (!preq) return;

    json data;
    data["jokers"] = req["jokers"];
    lobby->sendToPlayer(preq->getCreator()->getClient(), success("SWAP_JOKERS", data));
    lobby->getServer()->getPersistentRequestManager()->complete(requestId);
  } else {
    for (json joker : req["jokers"]) {
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
    }

    preq_t preq = lobby->getServer()->getPersistentRequestManager()->create(client->getPlayer());
    json data;
    data["jokers"] = req["jokers"];
    data["request_id"] = preq->getId();
    player_t randomPlayer = lobby->getGame()->getRandomPlayer(client->getPlayer());
    lobby->sendToPlayer(randomPlayer->getClient(), success("SWAP_JOKERS", data));
  }
}

void TheCupEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["money"] = lobby->getGame()->getEliminated().size() * 8;
  lobby->sendToPlayer(client, success("MONEY", data));
}

void GreenSealEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["money"] = -1;
  lobby->sendToOthers(client, success("MONEY", data));
}

void EraserEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  player_t randomPlayer = lobby->getGame()->getRandomPlayer(client->getPlayer());
  lobby->sendToPlayer(randomPlayer->getClient(), success("HAND_SIZE", data));
}

void PaintBucketEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  json data;
  data["hand_size"] = -1;
  lobby->sendToOthers(client, success("HAND_SIZE", data));
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
      if (joker["k"].is_null()) throw std::invalid_argument("No joker key");
      preqData["results"]["jokers"].push_back(joker);
    }
    for (json card : req["cards"]) {
      if (card["k"].is_null()) throw std::invalid_argument("No card key");
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
    lobby->sendToPlayer(preq->getCreator()->getClient(), success("GET_CARDS_AND_JOKERS", preqData["results"]));
    lobby->getServer()->getPersistentRequestManager()->complete(preq->getId());
  } else {
    preq_t preq = lobby->getServer()->getPersistentRequestManager()->create(client->getPlayer());
    json preqData;
    preqData["contributed"] = json::object();
    preqData["contributed"][client->getPlayer()->getSteamId()] = true;
    preqData["results"] = json::object();
    preqData["results"]["jokers"] = json::array();
    preqData["results"]["cards"] = json::array();
    preq->setData(preqData);

    json data;
    data["request_id"] = preq->getId();
    lobby->sendToOthers(client, success("GET_CARDS_AND_JOKERS", data), true);
    for (player_t p : lobby->getGame()->getRemaining()) {
      if (!preqData["contributed"][p->getSteamId()].get<bool>()) return;
    }
    lobby->sendToPlayer(client, success("GET_CARDS_AND_JOKERS", preqData["results"]));
    lobby->getServer()->getPersistentRequestManager()->complete(preq->getId());
  }
}

void ReadyForBossEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  lobby->getGame()->prepareForBoss(client->getPlayer());
  if (lobby->getGame()->isBossReady()) {
    lobby->getGame()->setBossPhaseEnabled(true);
    lobby->broadcast(success("START_BOSS"), true);
  }
}

void EliminatedEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");

  lobby->getGame()->eliminate(client->getPlayer());
}

void DefeatedBossEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isVersus()) throw std::runtime_error("Not a versus game");
  if (!req["score"].is_number_float()) throw std::runtime_error("No score provided");

  lobby->getGame()->addScore(client->getPlayer(), req["score"].get<double>());
  if (lobby->getGame()->isScoringFinished()) {
    json data;
    data["leaderboard"] = lobby->getGame()->getLeaderboard();
    lobby->getGame()->setBossPhaseEnabled(false);
    lobby->broadcast(success("LEADERBOARD", data), true);
  }
}
