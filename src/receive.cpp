#include "logs.hpp"
#include "server.hpp"

void client_thread(Server* server, client_t client) {
  if (!server->getNetworkManager()->handshake(client)) {
    server->lock();
    logger::error("TLS handshake failed for %s", client->getIP().c_str());
    server->disconnect(client);
    server->unlock();
    return;
  }

  while (true) {
    json req = server->getNetworkManager()->receive(client);
    if (req == json() || !req["cmd"].is_string()) break;

    server->lock();
    bool success = server->getEventListener()->process(client, req);
    server->unlock();

    if (!success) break;

    // try {
    //   server->lock();
    //   string command = req["cmd"].get<string>();

    //   if (!server->hasAlreadyJoined(client)) {
    //     if (command == "JOIN" && req["steam_id"].is_string() && req["unlock_hash"].is_string()) {
    //       string steamId = req["steam_id"].get<string>();
    //       string unlockHash = req["unlock_hash"].get<string>();
    //       client->setPlayer(std::make_shared<Player>(steamId, unlockHash));
    //       if (server->canJoin(client)) {
    //         server->join(client);
    //       } else {
    //         server->unlock();
    //         break;
    //       }
    //     } else {
    //       server->unlock();
    //       break;
    //     }
    //   } else if (command == "START") {
    //     server->stop();
    //     string seed = req["seed"].get<string>();
    //     string deck = req["deck"].get<string>();
    //     int stake = req["stake"].get<int>();
    //     bool versus = req["versus"].get<bool>();
    //     server->start(client->getPlayer(), seed, deck, stake, versus);

    //   } else if (command == "HIGHLIGHT") {
    //     string selectType = req["type"].get<string>();
    //     int index = req["index"].get<int>();
    //     server->highlight(client->getPlayer(), selectType, index);

    //   } else if (command == "UNHIGHLIGHT") {
    //     string selectType = req["type"].get<string>();
    //     int index = req["index"].get<int>();
    //     server->unhighlight(client->getPlayer(), selectType, index);

    //   } else if (command == "UNHIGHTLIGHT_ALL") {
    //     server->unhighlightAll(client->getPlayer());

    //   } else if (command == "PLAY_HAND") {
    //     server->playHand(client->getPlayer());
        
    //   } else if (command == "DISCARD_HAND") {
    //     server->discardHand(client->getPlayer());

    //   } else if (command == "SORT_HAND") {
    //     string sortType = req["type"].get<string>();
    //     server->sortHand(client->getPlayer(), sortType);

    //   } else if (command == "SELECT_BLIND") {
    //     server->selectBlind(client->getPlayer());

    //   } else if (command == "SKIP_BLIND") {
    //     server->skipBlind(client->getPlayer());

    //   } else if (command == "SELL") {
    //     string selectType = req["type"].get<string>();
    //     int index = req["index"].get<int>();
    //     server->sell(client->getPlayer(), selectType, index);

    //   } else if (command == "USE") {
    //     int index = req["index"].get<int>();
    //     server->use(client->getPlayer(), index);

    //   } else if (command == "BUY") {
    //     string selectType = req["type"].get<string>();
    //     int index = req["index"].get<int>();
    //     server->buy(client->getPlayer(), selectType, index);

    //   } else if (command == "BUY_AND_USE") {
    //     int index = req["index"].get<int>();
    //     server->buyAndUse(client->getPlayer(), index);

    //   } else if (command == "SKIP_BOOSTER") {
    //     server->skipBooster(client->getPlayer());
      
    //   } else if (command == "REROLL") {
    //     server->reroll(client->getPlayer());

    //   } else if (command == "NEXT_ROUND") {
    //     server->nextRound(client->getPlayer());

    //   } else if (command == "GO_TO_SHOP") {
    //     server->goToShop(client->getPlayer());

    //   } else if (command == "REORDER") {
    //     string selectType = req["type"].get<string>();
    //     int from = req["from"].get<int>();
    //     int to = req["to"].get<int>();
    //     server->reorder(client->getPlayer(), selectType, from, to);

    //   } else if (command == "ENDLESS") {
    //     server->endless();

    //   } else if (command == "SWAP_JOKERS") {
    //     json jokers = req["jokers"];
    //     if (req["request_id"].is_string()) {
    //       string requestId = req["request_id"].get<string>();
    //       server->swapJokers(client->getPlayer(), jokers, requestId);
    //     } else {
    //       server->swapJokers(client->getPlayer(), jokers);
    //     }

    //   } else if (command == "THE_CUP") {
    //     server->changeMoney(client->getPlayer(), server->getEliminatedPlayers().size() * 8);

    //   } else if (command == "GREEN_SEAL") {
    //     server->changeOthersMoney(client->getPlayer(), -1);

    //   } else if (command == "ERASER" || command == "PAINT_BUCKET") {
    //     server->changeHandSize(client->getPlayer(), -1, command == "ERASER");

    //   } else if (command == "GET_CARDS_AND_JOKERS") {
    //     if (req["request_id"].is_string()) {
    //       json jokers = req["jokers"];
    //       json cards = req["cards"];
    //       string requestId = req["request_id"].get<string>();
    //       server->getCardsAndJokers(client->getPlayer(), jokers, cards, requestId);
    //     } else {
    //       server->getCardsAndJokers(client->getPlayer());
    //     }

    //   } else if (command == "READY_FOR_BOSS") {
    //     server->readyForBoss(client->getPlayer());

    //   } else if (command == "ELIMINATED") {
    //     server->eliminate(client->getPlayer());

    //   } else if (command == "DEFEATED_BOSS") {
    //     double score = req["score"].get<double>();
    //     server->defeatedBoss(client->getPlayer(), score);

    //   }
    //   server->unlock();
    // } catch (...) {
    //   server->unlock();
    //   break;
    // }
  }

  server->lock();
  string ip = client->getIP();
  server->disconnect(client);
  logger::info("Client from %s disconnected", ip.c_str());
  server->unlock();
}