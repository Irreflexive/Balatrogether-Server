#ifndef BALATROGETHER_PREQ_H
#define BALATROGETHER_PREQ_H

#include <thread>
#include "json.hpp"
#include "player.hpp"

using json = nlohmann::json;

class PersistentRequest {
  public:
    std::string getId();
    player_t getCreator();
    json getData();
    void setData(json data);
  private:
    friend class PersistentRequestManager;
    PersistentRequest(player_t creator);
    std::string id;
    player_t original;
    json data;
    clock_t created;
};

class PersistentRequestManager {
  public:
    PersistentRequestManager(int requestLifetime = 10, int collectionInterval = 60);
    ~PersistentRequestManager();
    PersistentRequest* create(player_t creator);
    PersistentRequest* getById(std::string requestId);
    void complete(std::string requestId);
    void clearUnresolved();
  private:
    std::unordered_map<std::string, PersistentRequest*> requests;
    int requestLifetime;
    int collectionInterval;
};

#endif