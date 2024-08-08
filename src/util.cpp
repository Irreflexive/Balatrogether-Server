#include <limits.h>
#include <stdlib.h>
#include "util.hpp"

// Returns the directory that the program being executed is in
std::string getpath()
{
  char result[ PATH_MAX ];
  realpath("/proc/self/exe", result);
  std::string path = std::string(result);
  return path.substr(0, path.find_last_of('/'));
}

// Constructs a JSON object that is interpreted as valid by the client
json success(std::string cmd, json data)
{
  json res;
  res["success"] = true;
  res["cmd"] = cmd;
  res["data"] = data;
  return res;
}

// Constructs a JSON object for a successful operation with no data
json success(std::string cmd)
{
  return success(cmd, json::object());
}

// Constructs a JSON object that will make the client disconnect
json error(std::string msg)
{
  json res;
  res["success"] = false;
  res["error"] = msg;
  return res;
}