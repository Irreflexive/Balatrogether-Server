#ifndef BALATROGETHER_PREQ_H
#define BALATROGETHER_PREQ_H

#include "types.hpp"

using namespace balatrogether;

#include "player.hpp"

class balatrogether::PersistentRequest {
  public:
    string getId();
    player_t getCreator();
    json getData();
    void setData(json data);
  private:
    friend class PersistentRequestManager;
    PersistentRequest(player_t creator);
    string id;
    player_t original;
    json data;
    clock_t created;
};

class balatrogether::PersistentRequestManager {
  public:
    PersistentRequestManager(int requestLifetime = 10, int collectionInterval = 60);
    ~PersistentRequestManager();
    preq_t create(player_t creator);
    preq_t getById(string requestId);
    void complete(string requestId);
    void clearUnresolved();
  private:
    std::unordered_map<string, preq_t> requests;
    int requestLifetime;
    int collectionInterval;
};

#endif