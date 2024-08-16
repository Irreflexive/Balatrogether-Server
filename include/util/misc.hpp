#ifndef BALATROGETHER_MISC_UTIL_H
#define BALATROGETHER_MISC_UTIL_H

#include "types.hpp"

namespace balatrogether {
  class client_exception : public std::runtime_error {
    public:
      client_exception(const char* msg, bool disconnect = false) : std::runtime_error(msg), disconnect(disconnect) {};
      bool keep() { return !this->disconnect; };
    private:
      bool disconnect;
  };

  struct player_auth {
    steamid_t steamId;
    string unlockHash;
    std::unordered_map<string, int> stakes;
  };

  string getpath();
}

#endif