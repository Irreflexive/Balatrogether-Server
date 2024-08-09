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
  // TODO: implement
}

void UnhighlightCardEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void UnhighlightAllEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void PlayHandEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void DiscardHandEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void SortHandEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void ReorderCardsEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void SelectBlindEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void SkipBlindEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void SellCardEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void BuyCardEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void UseCardEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void BuyAndUseCardEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void SkipBoosterEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void RerollEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void NextRoundEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void GoToShopEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}

void EndlessEvent::execute(server_t server, client_t client, json req)
{
  // TODO: implement
}
