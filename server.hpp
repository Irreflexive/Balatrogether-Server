#ifndef BALATROGETHER_SERVER_H
#define BALATROGETHER_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "json.hpp"

#define BUFFER_SIZE 4096

using json = nlohmann::json;

struct Player {
  int fd;
  struct sockaddr_in addr;
  uint64_t steamId;
  friend bool operator==(Player const &lhs, Player const &rhs);
  friend bool operator!=(Player const &lhs, Player const &rhs);
};

class Game {
  public:
    Game();
    void start(bool versus);
    void stop();
    bool isRunning();
    bool isVersus();
    bool isCoop();
    std::vector<Player> getRemainingPlayers(std::vector<Player> connected);
    std::vector<Player> getEliminatedPlayers();
    void eliminate(Player p);
  private:
    bool inGame;
    bool versus;
    std::vector<Player> eliminated;
};

class Server {
  public:
    Server(int maxPlayers, bool debugMode);

    void join(Player p);
    void disconnect(Player p);
    bool hasAlreadyJoined(Player p);
    bool canJoin(Player p);

    void sendToPlayer(Player receiver, json payload);
    void sendToOthers(Player sender, json payload);
    void broadcast(json payload);
    json receive(Player sender);

    bool isHost(Player p);
    Player getHost();

    void start(Player sender, std::string seed, std::string deck, int stake, bool versus);
    void stop();
    void endless();

    void highlight(Player sender, std::string selectType, int index);
    void unhighlight(Player sender, std::string selectType, int index);
    void unhighlightAll(Player sender);
    void playHand(Player sender);
    void discardHand(Player sender);
    void sortHand(Player sender, std::string sortType);
    void reorder(Player sender, std::string selectType, int from, int to);

    void selectBlind(Player sender);
    void skipBlind(Player sender);

    void sell(Player sender, std::string selectType, int index);
    void use(Player sender, int index);
    void buy(Player sender, std::string selectType, int index);
    void buyAndUse(Player sender, int index);
    void skipBooster(Player sender);

    void reroll(Player sender);
    void nextRound(Player sender);
    void goToShop(Player sender);

    void annieAndHallie(Player sender, json jokers, bool responding, json isTargetResponse);
    void theCup(Player sender);

    json toJSON();
    void lock();
    void unlock();
  private:
    std::vector<Player> players;
    pthread_mutex_t mutex;
    int maxPlayers;
    Game game;
    bool debugMode;
};

#endif