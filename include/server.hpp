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
#include "network.hpp"

using std::string;
using json = nlohmann::json;

class Server;

typedef Server* server_t;
typedef EventListener<server_t>* server_listener_t;

class Server {
  public:
    Server(int port);
    ~Server();

    void acceptClient();
    void join(client_t c);
    void disconnect(client_t c);
    bool hasAlreadyJoined(client_t c);
    bool canJoin(client_t c);

    void sendToPlayer(client_t receiver, json payload);
    void sendToRandom(client_t sender, json payload);
    void sendToOthers(client_t sender, json payload, bool ignoreEliminated = false);
    void broadcast(json payload, bool ignoreEliminated = false);

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

    // Getters
    network_t getNetworkManager();
    config_t getConfig();
    server_listener_t getEventListener();
  private:
    friend class NetworkEvent<server_t>;
    network_t net;
    server_listener_t listener;
    client_list_t clients;
    std::mutex mutex;
    config_t config;
    Game game;
    PersistentRequestManager persistentRequests;
    int sockfd;
};

void client_thread(Server *server, client_t client);

#endif