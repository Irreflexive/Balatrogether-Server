#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H

#include <string>
#include "json.hpp"

using json = nlohmann::json;

std::string getpath();
json success(std::string cmd, json data);
json success(std::string cmd);
json error(std::string msg);

#endif