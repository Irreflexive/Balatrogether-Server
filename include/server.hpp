#ifndef BALATROGETHER_SERVER_H
#define BALATROGETHER_SERVER_H

#include <stdio.h>
#include <stdlib.h>
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

class Server {
  public:
    Server(int port);
    ~Server();

    void acceptClient();
    bool handshake(client_t c);
    void join(client_t c);
    void disconnect(client_t c);
    bool hasAlreadyJoined(client_t c);
    bool canJoin(client_t c);

    void sendToPlayer(client_t receiver, json payload);
    void sendToRandom(client_t sender, json payload);
    void sendToOthers(client_t sender, json payload, bool ignoreEliminated = false);
    void broadcast(json payload, bool ignoreEliminated = false);
    json receive(client_t sender);

    // Game state methods
    void start(player_t sender, string seed, string deck, int stake, bool versus);
    void stop();
    bool isRunning();
    bool isVersus();
    bool isCoop();
    bool isHost(player_t p);
    player_t getHost();
    player_list_t getPlayers();
    player_list_t getRemainingPlayers();
    player_list_t getEliminatedPlayers();

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
    int infoLog(string format, ...);
    int debugLog(string format, ...);
    int errorLog(string format, ...);
    
    Config config;
  private:
    void collectRequests();
    int log(string format, va_list args, string color = "0", FILE *fd = stdout);
    SSL_CTX* ssl_ctx = nullptr;
    client_list_t clients;
    std::mutex mutex;
    Game game;
    PersistentRequestManager persistentRequests;
    std::thread requestCollector;
    int sockfd;
};

void client_thread(Server *server, client_t client);

#endif