#include "events/server.hpp"

void JoinEvent::execute(server_t server, client_t client, json req)
{
  if (!req["steam_id"].is_string()) throw std::invalid_argument("No Steam ID provided");
  if (!req["unlock_hash"].is_string()) throw std::invalid_argument("No unlock provided");

  string steamId = req["steam_id"].get<string>();
  string unlockHash = req["unlock_hash"].get<string>();
  client->setPlayer(std::make_shared<Player>(steamId, unlockHash));
  if (!server->canJoin(client)) throw std::runtime_error("Cannot join server");
  
  server->join(client);
}

void StartRunEvent::execute(server_t server, client_t client, json req)
{
  if (!req["seed"].is_string()) throw std::invalid_argument("No seed provided");
  if (!req["deck"].is_string()) throw std::invalid_argument("No deck provided");
  if (!req["stake"].is_number_integer()) throw std::invalid_argument("No stake provided");
  if (!req["versus"].is_boolean()) throw std::invalid_argument("No game mode provided");

  server->stop();
  string seed = req["seed"].get<string>();
  string deck = req["deck"].get<string>();
  int stake = req["stake"].get<int>();
  bool versus = req["versus"].get<bool>();
  server->start(client->getPlayer(), seed, deck, stake, versus);
}

void HighlightCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  string selectType = req["type"].get<string>();
  int index = req["index"].get<int>();
  server->highlight(client->getPlayer(), selectType, index);
}

void UnhighlightCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  string selectType = req["type"].get<string>();
  int index = req["index"].get<int>();
  server->unhighlight(client->getPlayer(), selectType, index);
}

void UnhighlightAllEvent::execute(server_t server, client_t client, json req)
{
  server->unhighlightAll(client->getPlayer());
}

void PlayHandEvent::execute(server_t server, client_t client, json req)
{
  server->playHand(client->getPlayer());
}

void DiscardHandEvent::execute(server_t server, client_t client, json req)
{
  server->discardHand(client->getPlayer());
}

void SortHandEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No sort type provided");

  string sortType = req["type"].get<string>();
  server->sortHand(client->getPlayer(), sortType);
}

void ReorderCardsEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["from"].is_number_integer()) throw std::invalid_argument("No from index provided");
  if (!req["to"].is_number_integer()) throw std::invalid_argument("No to index provided");

  string selectType = req["type"].get<string>();
  int from = req["from"].get<int>();
  int to = req["to"].get<int>();
  server->reorder(client->getPlayer(), selectType, from, to);
}

void SelectBlindEvent::execute(server_t server, client_t client, json req)
{
  server->selectBlind(client->getPlayer());
}

void SkipBlindEvent::execute(server_t server, client_t client, json req)
{
  server->skipBlind(client->getPlayer());
}

void SellCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  string selectType = req["type"].get<string>();
  int index = req["index"].get<int>();
  server->sell(client->getPlayer(), selectType, index);
}

void BuyCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["type"].is_string()) throw std::invalid_argument("No card type provided");
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  string selectType = req["type"].get<string>();
  int index = req["index"].get<int>();
  server->buy(client->getPlayer(), selectType, index);
}

void UseCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  int index = req["index"].get<int>();
  server->use(client->getPlayer(), index);
}

void BuyAndUseCardEvent::execute(server_t server, client_t client, json req)
{
  if (!req["index"].is_number_integer()) throw std::invalid_argument("No index provided");

  int index = req["index"].get<int>();
  server->buyAndUse(client->getPlayer(), index);
}

void SkipBoosterEvent::execute(server_t server, client_t client, json req)
{
  server->skipBooster(client->getPlayer());
}

void RerollEvent::execute(server_t server, client_t client, json req)
{
  server->reroll(client->getPlayer());
}

void NextRoundEvent::execute(server_t server, client_t client, json req)
{
  server->nextRound(client->getPlayer());
}

void GoToShopEvent::execute(server_t server, client_t client, json req)
{
  server->goToShop(client->getPlayer());
}

void EndlessEvent::execute(server_t server, client_t client, json req)
{
  server->endless(client->getPlayer());
}
