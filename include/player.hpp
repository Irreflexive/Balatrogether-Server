#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include <memory>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>

class Client;
class Player;

typedef std::string steamid_t;
typedef std::vector<steamid_t> steamid_list_t;

typedef Client* client_t;
typedef std::vector<client_t> client_list_t;

typedef std::shared_ptr<Player> player_t;
typedef std::vector<player_t> player_list_t;

class Client {
  public:
    Client(int fd, sockaddr_in addr);
    ~Client();

    int getFd();
    std::string getIdentity();
    std::string getIP();
    SSL *getSSL();
    void setSSL(SSL *ssl);
    player_t getPlayer();
    void setPlayer(player_t player);
  private:
    friend class Player;
    int fd;
    struct sockaddr_in addr;
    player_t player;
    SSL *ssl = nullptr;
};

class Player {
  public:
    Player(steamid_t steamId, std::string unlockHash);

    client_t getClient();
    steamid_t getSteamId();
    std::string getUnlocks();
  private:
    friend class Client;
    steamid_t steamId = "Unknown";
    std::string unlockHash;
    client_t client = nullptr;
};

#endif