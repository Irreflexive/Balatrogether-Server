#include "server.hpp"

#define PORT 7063
#define MAX_PLAYERS 8

struct thread_arg {
  Server* server;
  Player client;
};

void* client_thread(void* arg) {
  thread_arg* info = (thread_arg*) arg;
  Server* server = info->server;
  Player client = info->client;

  if (DEBUG) std::cout << "Performing handshake" << std::endl;
  if (!server->handshake(&client)) {
    server->disconnect(client);
    pthread_exit(0);
  } else {
    if (DEBUG) std::cout << "AES: " << client.aesKey << " " << client.aesIV << std::endl;
  }

  json req = server->receive(client);
  if (req == json() || !req["cmd"].is_string() || !req["steam_id"].is_string()) {
    server->disconnect(client);
    pthread_exit(0);
  }
  std::string command = req["cmd"].get<std::string>();
  if (command == "JOIN") {
    std::string steamId = req["steam_id"].get<std::string>();
    client.steamId = strtoull(steamId.c_str(), NULL, 10);
    if (server->canJoin(client)) {
      server->join(client);
    } else {
      server->disconnect(client);
      pthread_exit(0);
    }
  } else {
    server->disconnect(client);
    pthread_exit(0);
  }

  while (true) {
    json req = server->receive(client);
    if (req == json() || !req["cmd"].is_string()) break;

    try {
      server->lock();
      std::string command = req["cmd"].get<std::string>();
      if (command == "START") {
        server->stop();
        std::string seed = req["seed"].get<std::string>();
        std::string deck = req["deck"].get<std::string>();
        int stake = req["stake"].get<int>();
        bool versus = req["versus"].get<bool>();
        server->start(client, seed, deck, stake, versus);

      } else if (command == "HIGHLIGHT") {
        std::string selectType = req["type"].get<std::string>();
        int index = req["index"].get<int>();
        server->highlight(client, selectType, index);

      } else if (command == "UNHIGHLIGHT") {
        std::string selectType = req["type"].get<std::string>();
        int index = req["index"].get<int>();
        server->unhighlight(client, selectType, index);

      } else if (command == "UNHIGHTLIGHT_ALL") {
        server->unhighlightAll(client);

      } else if (command == "PLAY_HAND") {
        server->playHand(client);
        
      } else if (command == "DISCARD_HAND") {
        server->discardHand(client);

      } else if (command == "SORT_HAND") {
        std::string sortType = req["type"].get<std::string>();
        server->sortHand(client, sortType);

      } else if (command == "SELECT_BLIND") {
        server->selectBlind(client);

      } else if (command == "SKIP_BLIND") {
        server->skipBlind(client);

      } else if (command == "SELL") {
        std::string selectType = req["type"].get<std::string>();
        int index = req["index"].get<int>();
        server->sell(client, selectType, index);

      } else if (command == "USE") {
        int index = req["index"].get<int>();
        server->use(client, index);

      } else if (command == "BUY") {
        std::string selectType = req["type"].get<std::string>();
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
        std::string selectType = req["type"].get<std::string>();
        int from = req["from"].get<int>();
        int to = req["to"].get<int>();
        server->reorder(client, selectType, from, to);

      } else if (command == "ENDLESS") {
        server->endless();

      } else if (command == "ANNIE_AND_HALLIE") {
        json jokers = req["jokers"];
        bool responding = req["responding"].get<bool>();
        if (responding) {
          std::string targetId = req["player"].get<std::string>();
          server->annieAndHallie(client, jokers, targetId);
        } else {
          server->annieAndHallie(client, jokers);
        }

      } else if (command == "THE_CUP") {
        server->theCup(client);

      } else if (command == "GREEN_SEAL") {
        server->greenSeal(client);

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
      server->disconnect(client);
      pthread_exit(0);
    }
  }

  server->disconnect(client);
  pthread_exit(0);
}

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(7063);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  Server* server = new Server(MAX_PLAYERS);

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    fprintf(stderr, "Failed to set reuse address\n");
    exit(1);
  }

  int bindStatus = bind(sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindStatus < 0) {
    fprintf(stderr, "Failed to bind\n");
    exit(1);
  }
  
  if (listen(sockfd, 3) < 0) {
    fprintf(stderr, "Failed to listen\n");
    exit(1);
  }

  printf("Balatrogether is listening on port %i\n", PORT);
  while (true) {
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int clientfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);
    Player p;
    p.fd = clientfd;
    p.addr = cli_addr;
    thread_arg args = {server, p};
    pthread_t thread;
    pthread_create(&thread, 0, client_thread, &args);
  }
}