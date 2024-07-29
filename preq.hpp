#ifndef BALATROGETHER_PREQ_H
#define BALATROGETHER_PREQ_H

#include "json.hpp"
#include "player.hpp"

using json = nlohmann::json;

class PersistentRequest {
  public:
    std::string getId();
    Player* getCreator();
    json getData();
    void setData(json data);
  private:
    friend class PersistentRequestManager;
    PersistentRequest(Player* creator);
    std::string id;
    Player* original;
    json data;
    clock_t created;
};

class PersistentRequestManager {
  public:
    ~PersistentRequestManager();
    PersistentRequest* create(Player* creator);
    PersistentRequest* getById(std::string requestId);
    void complete(std::string requestId);
  private:
    std::unordered_map<std::string, PersistentRequest*> requests;
};

#endif