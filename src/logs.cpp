#include <sstream>
#include <chrono>
#include <iomanip>
#include <stdarg.h>
#include "logs.hpp"

bool debugOutput = false;

void logger::setDebugOutputEnabled(bool enabled)
{
  debugOutput = enabled;
}

int log(string format, va_list args, string color = "0", FILE *fd = stdout)
{
  std::stringstream modifiedFormat;
  std::time_t currentTime = std::time(nullptr);
  modifiedFormat << "\033[" << color << "m[" << std::put_time(std::gmtime(&currentTime), "%FT%TZ") << "] " << format << "\033[0m" << std::endl;
  int n = vfprintf(fd, modifiedFormat.str().c_str(), args);
  return n;

}

// Prints a new line with timestamp and [INFO] prefix
int logger::info(string format, ...)
{
  string fmt = "[INFO] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args);
  va_end(args);
  return n;
}

// Prints a new line with timestamp and [DEBUG] prefix, only if debug mode is enabled
int logger::debug(string format, ...)
{
  if (!debugOutput) return 0;
  string fmt = "[DEBUG] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args, "33");
  va_end(args);
  return n;
}

// Prints an error message with timestamp
int logger::error(string format, ...)
{
  string fmt = "[ERROR] " + format;
  va_list args;
  va_start(args, format);
  int n = log(fmt, args, "31", stderr);
  va_end(args);
  return n;
}
