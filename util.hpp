#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H
#include <sstream>

std::string uint64_to_string( uint64_t value ) {
  std::ostringstream os;
  os << value;
  return os.str();
}

#endif