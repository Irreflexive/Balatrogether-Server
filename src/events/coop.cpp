#include "util.hpp"
#include "events/coop.hpp"

void HighlightCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->sendToOthers(client, success("HIGHLIGHT", data));
}

void UnhighlightCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->sendToOthers(client, success("UNHIGHLIGHT", data));
}

void UnhighlightAllEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->sendToOthers(client, success("UNHIGHLIGHT_ALL"));
}

void PlayHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("PLAY_HAND"));
}

void DiscardHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("DISCARD_HAND"));
}

void SortHandEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No sort type provided");
  string sortType = req["type"].get<string>();
  if (sortType != "suit" && sortType != "value") throw std::invalid_argument("Invalid sort type");

  json data;
  data["type"] = sortType;
  lobby->broadcast(success("SORT_HAND", data));
}

void ReorderCardsEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["from"].is_number_integer()) throw std::invalid_argument("No from index provided");
  if (!req["to"].is_number_integer()) throw std::invalid_argument("No to index provided");

  json data;
  data["type"] = req["type"];
  data["from"] = req["from"];
  data["to"] = req["to"];
  lobby->sendToOthers(client, success("REORDER", data));
}

void SelectBlindEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("SELECT_BLIND"));
}

void SkipBlindEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("SKIP_BLIND"));
}

void SellCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->broadcast(success("SELL", data));
}

void BuyCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  lobby->broadcast(success("BUY", data));
}

void UseCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  lobby->broadcast(success("USE", data));
}

void BuyAndUseCardEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  lobby->broadcast(success("BUY_AND_USE", data));
}

void SkipBoosterEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("SKIP_BOOSTER"));
}

void RerollEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("REROLL"));
}

void NextRoundEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("NEXT_ROUND"));
}

void GoToShopEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("GO_TO_SHOP"));
}

void EndlessEvent::execute(lobby_t lobby, client_t client, json req)
{
  if (!lobby->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  lobby->broadcast(success("ENDLESS"));
}