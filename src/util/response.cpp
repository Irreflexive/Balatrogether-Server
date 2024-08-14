#include "util/response.hpp"

using namespace balatrogether;

// Constructs a JSON object that is interpreted as valid by the client
json response::success(string cmd, json data)
{
  json res;
  res["success"] = true;
  res["cmd"] = cmd;
  res["data"] = data;
  return res;
}

// Constructs a JSON object for a successful operation with no data
json response::success(string cmd)
{
  return success(cmd, json::object());
}

// Constructs a JSON object that will make the client disconnect
json response::error(string msg)
{
  json res;
  res["success"] = false;
  res["error"] = msg;
  return res;
}