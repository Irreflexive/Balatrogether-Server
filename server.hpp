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

#include "json.hpp"
#include "encrypt.hpp"
#include "player.hpp"
#include "game.hpp"
#include "preq.hpp"

#define BUFFER_SIZE 16384
#define ENCRYPT true
#define DEBUG true

using std::string;
using json = nlohmann::json;

json success(string cmd, json data);
json success(string cmd);
json error(string msg);

class Server {
  public:
    Server(int maxPlayers);
    ~Server();

    bool handshake(Player* p);
    void join(Player* p);
    void disconnect(Player* p);
    bool hasAlreadyJoined(Player* p);
    bool canJoin(Player* p);

    void sendToPlayer(Player* receiver, json payload);
    void sendToRandom(Player* sender, json payload);
    void sendToOthers(Player* sender, json payload, bool ignoreEliminated = false);
    void broadcast(json payload, bool ignoreEliminated = false);
    json receive(Player* sender);

    bool isHost(Player* p);
    Player* getHost();

    // Game state methods
    void start(Player* sender, string seed, string deck, int stake, bool versus);
    void stop();
    bool isRunning();
    bool isVersus();
    bool isCoop();
    player_list_t getRemainingPlayers();
    player_list_t getEliminatedPlayers();

    // Co-op network events
    void endless();

    void highlight(Player* sender, string selectType, int index);
    void unhighlight(Player* sender, string selectType, int index);
    void unhighlightAll(Player* sender);
    void playHand(Player* sender);
    void discardHand(Player* sender);
    void sortHand(Player* sender, string sortType);
    void reorder(Player* sender, string selectType, int from, int to);

    void selectBlind(Player* sender);
    void skipBlind(Player* sender);

    void sell(Player* sender, string selectType, int index);
    void use(Player* sender, int index);
    void buy(Player* sender, string selectType, int index);
    void buyAndUse(Player* sender, int index);
    void skipBooster(Player* sender);

    void reroll(Player* sender);
    void nextRound(Player* sender);
    void goToShop(Player* sender);

    // Versus network events
    void swapJokers(Player* sender, json jokers);
    void swapJokers(Player* sender, json jokers, string requestId);
    void changeMoney(Player* sender, int change);
    void changeOthersMoney(Player* sender, int change);
    void changeHandSize(Player* sender, int change, bool chooseRandom);
    void getCardsAndJokers(Player* sender);
    void getCardsAndJokers(Player* sender, json jokers, json cards, string requestId);
    void readyForBoss(Player* sender);
    void eliminate(Player* p);
    void defeatedBoss(Player* p, double score);

    // State management
    json getState();
    json toJSON();
    void lock();
    void unlock();
  private:
    SSL_CTX* ssl_ctx = nullptr;
    player_list_t players;
    pthread_mutex_t mutex;
    int maxPlayers;
    Game game;
    PersistentRequestManager persistentRequests;
};

#endif