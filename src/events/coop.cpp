#include "util/response.hpp"
#include "util/validation.hpp"
#include "events/coop.hpp"
#include "lobby.hpp"

using namespace balatrogether;

void HighlightCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 32)) throw std::invalid_argument("No card type provided");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->sendToOthers(client, response::success("HIGHLIGHT", data));
}

void UnhighlightCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 32)) throw std::invalid_argument("No card type provided");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->sendToOthers(client, response::success("UNHIGHLIGHT", data));
}

void UnhighlightAllEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->sendToOthers(client, response::success("UNHIGHLIGHT_ALL"));
}

void PlayHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("PLAY_HAND"));
}

void DiscardHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("DISCARD_HAND"));
}

void SortHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 8)) throw std::invalid_argument("No sort type provided");
  string sortType = req["type"].get<string>();
  if (sortType != "suit" && sortType != "value") throw std::invalid_argument("Invalid sort type");

  json data;
  data["type"] = sortType;
  lobby->broadcast(response::success("SORT_HAND", data));
}

void ReorderCardsEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 32)) throw std::invalid_argument("No card type provided");
  if (!validation::integer(req["from"], 0)) throw std::invalid_argument("No from index provided");
  if (!validation::integer(req["to"], 0)) throw std::invalid_argument("No to index provided");

  json data;
  data["type"] = req["type"];
  data["from"] = req["from"];
  data["to"] = req["to"];
  lobby->sendToOthers(client, response::success("REORDER", data));
}

void SelectBlindEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("SELECT_BLIND"));
}

void SkipBlindEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("SKIP_BLIND"));
}

void SellCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 32)) throw std::invalid_argument("No card type provided");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->broadcast(response::success("SELL", data));
}

void BuyCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::string(req["type"], 1, 32)) throw std::invalid_argument("No card type provided");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->broadcast(response::success("BUY", data));
}

void UseCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  lobby->broadcast(response::success("USE", data));
}

void BuyAndUseCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::integer(req["index"], 0)) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  lobby->broadcast(response::success("BUY_AND_USE", data));
}

void SkipBoosterEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("SKIP_BOOSTER"));
}

void RerollEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("REROLL"));
}

void NextRoundEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("NEXT_ROUND"));
}

void GoToShopEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("GO_TO_SHOP"));
}

void EndlessEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(response::success("ENDLESS"));
}

void GameSpeedEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!validation::integer(req["speed"], 0, 32)) throw std::invalid_argument("Invalid game speed");

  json data;
  data["speed"] = req["speed"].get<int>();
  lobby->sendToOthers(client, response::success("GAME_SPEED", data));
}