#ifndef BALATROGETHER_PLAYER_H
#define BALATROGETHER_PLAYER_H

#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <unistd.h>

class Player {
  public:
    Player(int fd, sockaddr_in addr);
    ~Player();

    int getFd();
    uint64_t getSteamId();
    void setSteamId(uint64_t steamId);
    SSL *getSSL();
    void setSSL(SSL *ssl);

    operator std::string() const;
  private:
    int fd;
    struct sockaddr_in addr;
    uint64_t steamId = 0;
    SSL *ssl = nullptr;
};

typedef std::vector<Player*> player_list_t;
typedef std::pair<Player*, double> player_score_t;
typedef std::vector<player_score_t> leaderboard_t;

#endif