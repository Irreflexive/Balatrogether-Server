#include "server.hpp"

#define PORT 7063
#define MAX_PLAYERS 8
#define DEBUG true

struct thread_arg {
  Server* server;
  Player client;
};

void* client_thread(void* arg) {
  thread_arg* info = (thread_arg*) arg;
  Server* server = info->server;
  Player client = info->client;

  json req = server->receive(client);
  if (req == json()) {
    server->disconnect(client);
    pthread_exit(0);
  }
  std::string command = req["cmd"].template get<std::string>();
  if (command == "JOIN") {
    std::string steamId = req["steam_id"].template get<std::string>();
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
    if (req == json()) break;

    std::string command = req["cmd"].template get<std::string>();
    if (command == "START") {
      server->stop();
      std::string seed = req["seed"].template get<std::string>();
      std::string deck = req["deck"].template get<std::string>();
      int stake = req["stake"].template get<int>();
      server->start(client, seed, deck, stake);

    } else if (command == "HIGHLIGHT") {
      std::string selectType = req["type"].template get<std::string>();
      int index = req["index"].template get<int>();
      server->highlight(client, selectType, index);

    } else if (command == "UNHIGHLIGHT") {
      std::string selectType = req["type"].template get<std::string>();
      int index = req["index"].template get<int>();
      server->unhighlight(client, selectType, index);

    } else if (command == "UNHIGHTLIGHT_ALL") {
      server->unhighlightAll(client);

    } else if (command == "PLAY_HAND") {
      server->playHand(client);
      
    } else if (command == "DISCARD_HAND") {
      server->discardHand(client);

    } else if (command == "SORT_HAND") {
      std::string sortType = req["type"].template get<std::string>();
      server->sortHand(client, sortType);

    } else if (command == "SELECT_BLIND") {
      server->selectBlind(client);

    } else if (command == "SKIP_BLIND") {
      server->skipBlind(client);

    } else if (command == "SELL") {
      std::string selectType = req["type"].template get<std::string>();
      int index = req["index"].template get<int>();
      server->sell(client, selectType, index);

    } else if (command == "USE") {
      int index = req["index"].template get<int>();
      server->use(client, index);

    } else if (command == "BUY") {
      std::string selectType = req["type"].template get<std::string>();
      int index = req["index"].template get<int>();
      server->buy(client, selectType, index);

    } else if (command == "BUY_AND_USE") {
      int index = req["index"].template get<int>();
      server->buyAndUse(client, index);
    
    } else if (command == "REROLL") {
      server->reroll(client);

    } else if (command == "NEXT_ROUND") {
      server->nextRound(client);

    } else if (command == "GO_TO_SHOP") {
      server->goToShop(client);

    } else if (command == "REORDER") {
      std::string selectType = req["type"].template get<std::string>();
      int from = req["from"].template get<int>();
      int to = req["to"].template get<int>();
      server->reorder(client, selectType, from, to);

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

  Server* server = new Server(MAX_PLAYERS, DEBUG);

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