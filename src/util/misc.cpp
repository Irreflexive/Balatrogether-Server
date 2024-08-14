#include <limits.h>
#include "util/misc.hpp"

using namespace balatrogether;

// Returns the directory that the program being executed is in
string balatrogether::getpath()
{
  char result[PATH_MAX];
  realpath("/proc/self/exe", result);
  string path = string(result);
  return path.substr(0, path.find_last_of('/'));
}
