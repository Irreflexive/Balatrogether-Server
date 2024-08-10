#include <limits.h>
#include <stdlib.h>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <stdarg.h>
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

bool debugOutput = false;

// Enables or disables debug logs from being output to stdout
void logger::setDebugOutputEnabled(bool enabled)
{
  debugOutput = enabled;
}

int log(std::string format, va_list args, std::string color = "0", FILE *fd = stdout)
{
  std::stringstream modifiedFormat;
  std::time_t currentTime = std::time(nullptr);
  modifiedFormat << "\033[" << color << "m[" << std::put_time(std::gmtime(&currentTime), "%FT%TZ") << "] " << format << "\033[0m" << std::endl;
  int n = vfprintf(fd, modifiedFormat.str().c_str(), args);
  return n;
}

// Prints a new line with timestamp and [INFO] prefix
int logger::info(std::string format, ...)
{
  std::string fmt = "[INFO] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args);
  va_end(args);
  return n;
}

// Prints a new line with timestamp and [DEBUG] prefix, only if debug mode is enabled
int logger::debug(std::string format, ...)
{
  if (!debugOutput) return 0;
  std::string fmt = "[DEBUG] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args, "33");
  va_end(args);
  return n;
}

// Prints an error message with timestamp
int logger::error(std::string format, ...)
{
  std::string fmt = "[ERROR] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args, "31", stderr);
  va_end(args);
  return n;
}