#include "util.hpp"
#include "events/coop.hpp"

void HighlightCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  server->sendToOthers(client, success("HIGHLIGHT", data));
}

void UnhighlightCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  server->sendToOthers(client, success("UNHIGHLIGHT", data));
}

void UnhighlightAllEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->sendToOthers(client, success("UNHIGHLIGHT_ALL"));
}

void PlayHandEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("PLAY_HAND"));
}

void DiscardHandEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("DISCARD_HAND"));
}

void SortHandEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No sort type provided");
  string sortType = req["type"].get<string>();
  if (sortType != "suit" && sortType != "value") throw std::invalid_argument("Invalid sort type");

  json data;
  data["type"] = sortType;
  server->broadcast(success("SORT_HAND", data));
}

void ReorderCardsEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["from"].is_number_integer()) throw std::invalid_argument("No from index provided");
  if (!req["to"].is_number_integer()) throw std::invalid_argument("No to index provided");

  json data;
  data["type"] = req["type"];
  data["from"] = req["from"];
  data["to"] = req["to"];
  server->sendToOthers(client, success("REORDER", data));
}

void SelectBlindEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("SELECT_BLIND"));
}

void SkipBlindEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("SKIP_BLIND"));
}

void SellCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  server->broadcast(success("SELL", data));
}

void BuyCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["type"] = req["type"];
  data["index"] = req["index"];
  server->broadcast(success("BUY", data));
}

void UseCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  server->broadcast(success("USE", data));
}

void BuyAndUseCardEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  json data;
  data["index"] = req["index"];
  server->broadcast(success("BUY_AND_USE", data));
}

void SkipBoosterEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("SKIP_BOOSTER"));
}

void RerollEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("REROLL"));
}

void NextRoundEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("NEXT_ROUND"));
}

void GoToShopEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("GO_TO_SHOP"));
}

void EndlessEvent::execute(server_t server, client_t client, json req)
{
  if (!server->getGame()->isCoop()) throw std::runtime_error("Not a co-op game");

  server->broadcast(success("ENDLESS"));
}