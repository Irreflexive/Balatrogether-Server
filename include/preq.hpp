#ifndef BALATROGETHER_PREQ_H
#define BALATROGETHER_PREQ_H

#include "types.hpp"

namespace balatrogether {
  class PersistentRequest {
    public:
      string getId();
      steamid_t getCreator();
      json getData();
      void setData(json data);
    private:
      friend class PersistentRequestManager;
      PersistentRequest(steamid_t creator);
      string id;
      steamid_t original;
      json data;
      clock_t created;
  };

  class PersistentRequestManager {
    public:
      PersistentRequestManager(int requestLifetime = 10, int collectionInterval = 60);
      ~PersistentRequestManager();
      preq_t create(steamid_t creator);
      preq_t getById(string requestId);
      void complete(string requestId);
      void clearUnresolved();
    private:
      std::unordered_map<string, preq_t> requests;
      int requestLifetime;
      int collectionInterval;
  };
}

#endif