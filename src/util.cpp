#include <limits.h>
#include <stdlib.h>
#include "util.hpp"

std::string getpath()
{
  char result[ PATH_MAX ];
  realpath("/proc/self/exe", result);
  std::string path = std::string(result);
  return path.substr(0, path.find_last_of('/'));
}