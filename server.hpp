#ifndef BALATROGETHER_SERVER_H
#define BALATROGETHER_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <sstream>
#include <unistd.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
  #include <winsock2.h>
  #define closesocket closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #define closesocket close
#endif

#include "json.hpp"
#include "encrypt.hpp"
#include "player.hpp"
#include "game.hpp"
#include "preq.hpp"
#include "config.hpp"

#define BUFFER_SIZE 65536

using std::string;
using json = nlohmann::json;

json success(string cmd, json data);
json success(string cmd);
json error(string msg);

class Server {
  public:
    Server();
    ~Server();

    bool handshake(player_t p);
    void join(player_t p);
    void disconnect(player_t p);
    bool hasAlreadyJoined(player_t p);
    bool canJoin(player_t p);

    void sendToPlayer(player_t receiver, json payload);
    void sendToRandom(player_t sender, json payload);
    void sendToOthers(player_t sender, json payload, bool ignoreEliminated = false);
    void broadcast(json payload, bool ignoreEliminated = false);
    json receive(player_t sender);

    bool isHost(player_t p);
    player_t getHost();

    // Game state methods
    void start(player_t sender, string seed, string deck, int stake, bool versus);
    void stop();
    bool isRunning();
    bool isVersus();
    bool isCoop();
    player_list_t getRemainingPlayers();
    steamid_list_t getEliminatedPlayers();

    // Co-op network events
    void endless();

    void highlight(player_t sender, string selectType, int index);
    void unhighlight(player_t sender, string selectType, int index);
    void unhighlightAll(player_t sender);
    void playHand(player_t sender);
    void discardHand(player_t sender);
    void sortHand(player_t sender, string sortType);
    void reorder(player_t sender, string selectType, int from, int to);

    void selectBlind(player_t sender);
    void skipBlind(player_t sender);

    void sell(player_t sender, string selectType, int index);
    void use(player_t sender, int index);
    void buy(player_t sender, string selectType, int index);
    void buyAndUse(player_t sender, int index);
    void skipBooster(player_t sender);

    void reroll(player_t sender);
    void nextRound(player_t sender);
    void goToShop(player_t sender);

    // Versus network events
    void swapJokers(player_t sender, json jokers);
    void swapJokers(player_t sender, json jokers, string requestId);
    void changeMoney(player_t sender, int change);
    void changeOthersMoney(player_t sender, int change);
    void changeHandSize(player_t sender, int change, bool chooseRandom);
    void getCardsAndJokers(player_t sender);
    void getCardsAndJokers(player_t sender, json jokers, json cards, string requestId);
    void readyForBoss(player_t sender);
    void eliminate(player_t p);
    void defeatedBoss(player_t p, double score);

    // State management
    json getState();
    json toJSON();
    void lock();
    void unlock();

    // Logging functions
    int infoLog(std::string format, ...);
    int debugLog(std::string format, ...);
    int errorLog(std::string format, ...);
  private:
    friend void collectRequests(Server* server);
    int log(std::string format, va_list args, std::string color = "0", FILE *fd = stdout);
    SSL_CTX* ssl_ctx = nullptr;
    player_list_t players;
    std::mutex mutex;
    Config config;
    Game game;
    PersistentRequestManager persistentRequests;
    std::thread requestCollector;
};

#endif