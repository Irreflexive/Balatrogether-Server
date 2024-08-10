#include "util.hpp"
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

    //   if (command == "READY_FOR_BOSS") {
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