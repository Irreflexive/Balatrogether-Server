#ifndef BALATROGETHER_RESPONSE_UTIL_H
#define BALATROGETHER_RESPONSE_UTIL_H

#include "types.hpp"

namespace balatrogether::response {
  json success(string cmd, json data);
  json success(string cmd);
  json error(string msg);
};

#endif