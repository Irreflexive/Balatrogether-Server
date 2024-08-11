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

#include "types.hpp"

using namespace balatrogether;

#include "network.hpp"
#include "player.hpp"
#include "lobby.hpp"
#include "preq.hpp"
#include "config.hpp"

class balatrogether::Server {
  public:
    Server(int port);
    ~Server();

    void acceptClient();

    bool canConnect(client_t c);
    void connect(client_t c, steamid_t steamId, string unlockHash);
    void disconnect(client_t c);

    lobby_t createLobby();
    std::vector<string> getLobbyCodes();
    lobby_t getDefaultLobby();
    lobby_t getLobby(string code);
    void deleteLobby(lobby_t lobby);

    // State management
    void lock();
    void unlock();

    // Getters
    client_list_t getClients();
    network_t getNetworkManager();
    config_t getConfig();
    server_listener_t getEventListener();
    preq_manager_t getPersistentRequestManager();
  private:
    network_t net;
    server_listener_t listener;
    client_list_t clients;
    lobby_list_t lobbies;
    std::mutex mutex;
    config_t config;
    preq_manager_t persistentRequests;
    int sockfd;
};

void client_thread(server_t server, client_t client);

#endif