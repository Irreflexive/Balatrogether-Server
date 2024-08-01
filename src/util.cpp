#include "util.hpp"

std::string getpath()
{
  char result[ PATH_MAX ];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  std::string path = std::string(result, (count > 0) ? count : 0);
  return path.substr(0, path.find_last_of('/'));
}