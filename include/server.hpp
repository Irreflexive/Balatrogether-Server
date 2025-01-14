#ifndef BALATROGETHER_SERVER_H
#define BALATROGETHER_SERVER_H

#include <mutex>
#include "types.hpp"
#include "network.hpp"
#include "preq.hpp"
#include "config.hpp"
#include "listeners/server.hpp"
#include "listeners/console.hpp"

namespace balatrogether {
  class Server {
    public:
      Server(int port);
      ~Server();

      void acceptClient();

      bool canConnect(client_t c);
      void connect(client_t c, player_auth auth);
      void disconnect(client_t c);

      lobby_t getDefaultLobby();
      lobby_t getLobby(int index);
      lobby_list_t getLobbies();

      void lock();
      void unlock();

      client_list_t getClients();
      network_t getNetworkManager();
      config_t getConfig();
      server_listener_t getEventListener();
      console_listener_t getConsole();
      preq_manager_t getPersistentRequestManager();
    private:
      void setSocketOption(int level, int option, int value, const char *err_message = "Failed to set option");
      network_t net;
      server_listener_t listener;
      console_listener_t console;
      client_list_t clients;
      lobby_list_t lobbies;
      std::mutex mutex;
      config_t config;
      preq_manager_t persistentRequests;
      int sockfd;
  };

  void client_thread(server_t server, client_t client);
  void console_thread(server_t server);
}

#endif