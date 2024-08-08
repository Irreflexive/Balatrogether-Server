#include <csignal>
#include "server.hpp"
#include "console.hpp"

#define PORT 7063

Console* console;

void client_thread(Server* server, player_t client) {
  if (!server->handshake(client)) {
    server->lock();
    server->errorLog("TLS handshake failed for %s", client->getIP().c_str());
    server->disconnect(client);
    server->unlock();
    return;
  }

  while (true) {
    json req = server->receive(client);
    if (req == json() || !req["cmd"].is_string()) break;

    try {
      server->lock();
      string command = req["cmd"].get<string>();

      if (!server->hasAlreadyJoined(client)) {
        if (command == "JOIN" && req["steam_id"].is_string() && req["unlock_hash"].is_string()) {
          string steamId = req["steam_id"].get<string>();
          string unlockHash = req["unlock_hash"].get<string>();
          client->setSteamId(steamId);
          client->setUnlocks(unlockHash);
          if (server->canJoin(client)) {
            server->join(client);
          } else {
            server->unlock();
            break;
          }
        } else {
          server->unlock();
          break;
        }
      } else if (command == "START") {
        server->stop();
        string seed = req["seed"].get<string>();
        string deck = req["deck"].get<string>();
        int stake = req["stake"].get<int>();
        bool versus = req["versus"].get<bool>();
        server->start(client, seed, deck, stake, versus);

      } else if (command == "HIGHLIGHT") {
        string selectType = req["type"].get<string>();
        int index = req["index"].get<int>();
        server->highlight(client, selectType, index);

      } else if (command == "UNHIGHLIGHT") {
        string selectType = req["type"].get<string>();
        int index = req["index"].get<int>();
        server->unhighlight(client, selectType, index);

      } else if (command == "UNHIGHTLIGHT_ALL") {
        server->unhighlightAll(client);

      } else if (command == "PLAY_HAND") {
        server->playHand(client);
        
      } else if (command == "DISCARD_HAND") {
        server->discardHand(client);

      } else if (command == "SORT_HAND") {
        string sortType = req["type"].get<string>();
        server->sortHand(client, sortType);

      } else if (command == "SELECT_BLIND") {
        server->selectBlind(client);

      } else if (command == "SKIP_BLIND") {
        server->skipBlind(client);

      } else if (command == "SELL") {
        string selectType = req["type"].get<string>();
        int index = req["index"].get<int>();
        server->sell(client, selectType, index);

      } else if (command == "USE") {
        int index = req["index"].get<int>();
        server->use(client, index);

      } else if (command == "BUY") {
        string selectType = req["type"].get<string>();
        int index = req["index"].get<int>();
        server->buy(client, selectType, index);

      } else if (command == "BUY_AND_USE") {
        int index = req["index"].get<int>();
        server->buyAndUse(client, index);

      } else if (command == "SKIP_BOOSTER") {
        server->skipBooster(client);
      
      } else if (command == "REROLL") {
        server->reroll(client);

      } else if (command == "NEXT_ROUND") {
        server->nextRound(client);

      } else if (command == "GO_TO_SHOP") {
        server->goToShop(client);

      } else if (command == "REORDER") {
        string selectType = req["type"].get<string>();
        int from = req["from"].get<int>();
        int to = req["to"].get<int>();
        server->reorder(client, selectType, from, to);

      } else if (command == "ENDLESS") {
        server->endless();

      } else if (command == "SWAP_JOKERS") {
        json jokers = req["jokers"];
        if (req["request_id"].is_string()) {
          string requestId = req["request_id"].get<string>();
          server->swapJokers(client, jokers, requestId);
        } else {
          server->swapJokers(client, jokers);
        }

      } else if (command == "THE_CUP") {
        server->changeMoney(client, server->getEliminatedPlayers().size() * 8);

      } else if (command == "GREEN_SEAL") {
        server->changeOthersMoney(client, -1);

      } else if (command == "ERASER" || command == "PAINT_BUCKET") {
        server->changeHandSize(client, -1, command == "ERASER");

      } else if (command == "GET_CARDS_AND_JOKERS") {
        if (req["request_id"].is_string()) {
          json jokers = req["jokers"];
          json cards = req["cards"];
          string requestId = req["request_id"].get<string>();
          server->getCardsAndJokers(client, jokers, cards, requestId);
        } else {
          server->getCardsAndJokers(client);
        }

      } else if (command == "READY_FOR_BOSS") {
        server->readyForBoss(client);

      } else if (command == "ELIMINATED") {
        server->eliminate(client);

      } else if (command == "DEFEATED_BOSS") {
        double score = req["score"].get<double>();
        server->defeatedBoss(client, score);

      }
      server->unlock();
    } catch (...) {
      server->unlock();
      break;
    }
  }

  server->lock();
  std::string ip = client->getIP();
  server->disconnect(client);
  server->infoLog("Client from %s disconnected", ip.c_str());
  server->unlock();
}

int main() {
  Server* server = new Server();
  server->infoLog("Starting server");

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  server->infoLog("Configuring socket options");

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    server->errorLog("Failed to set reuse address");
    exit(1);
  }
  server->infoLog("Allowed address reuse");

  int send_size = BUFFER_SIZE;
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size)) < 0) {
    server->errorLog("Failed to set send buffer size");
    exit(1);
  }
  server->infoLog("Set send buffer size");

  int recv_size = BUFFER_SIZE;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size)) < 0) {
    server->errorLog("Failed to set receive buffer size");
    exit(1);
  }
  server->infoLog("Set receive buffer size");

  int keepalive = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
    server->errorLog("Failed to set keepalive");
    exit(1);
  }
  server->infoLog("Enabled keepalive timer");

  int bindStatus = bind(sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindStatus < 0) {
    server->errorLog("Failed to bind");
    exit(1);
  }
  server->infoLog("Bound to address");
  
  if (listen(sockfd, 3) < 0) {
    server->errorLog("Failed to listen");
    exit(1);
  }

  server->infoLog("Balatrogether is listening on port %i", PORT);
  console = new Console(server);
  std::thread(console_thread, console).detach();

  auto stopFunc = [](int s) { console->execute("stop", {}); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int clientfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);
    player_t p = new Player(clientfd, cli_addr);
    server->infoLog("Client from %s attempting to connect", p->getIP().c_str());
    std::thread(client_thread, server, p).detach();
  }

  closesocket(sockfd);
}