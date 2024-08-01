#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>

typedef std::string steamid_t;
typedef std::vector<steamid_t> steamid_list_t;

class Player {
  public:
    Player(int fd, sockaddr_in addr);
    ~Player();

    int getFd();
    std::string getIP();
    steamid_t getSteamId();
    void setSteamId(steamid_t steamId);
    std::string getUnlocks();
    void setUnlocks(std::string unlockHash);
    SSL *getSSL();
    void setSSL(SSL *ssl);
  private:
    int fd;
    struct sockaddr_in addr;
    steamid_t steamId = "Unknown";
    std::string unlockHash;
    SSL *ssl = nullptr;
};

typedef Player* player_t;
typedef std::vector<player_t> player_list_t;

#endif