#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H

#include <string>
#include "json.hpp"

using json = nlohmann::json;

std::string getpath();
json success(std::string cmd, json data);
json success(std::string cmd);
json error(std::string msg);

namespace logger {
  void setDebugOutputEnabled(bool enabled);

  int info(std::string format, ...);
  int debug(std::string format, ...);
  int error(std::string format, ...);
}

#endif