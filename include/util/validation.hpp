#ifndef BALATROGETHER_VALIDATION_UTIL_H
#define BALATROGETHER_VALIDATION_UTIL_H

#include <limits.h>
#include "types.hpp"

namespace balatrogether::validation {
  bool string(json& data);
  bool string(json& data, size_t maxLength);
  bool string(json& data, size_t minLength, size_t maxLength);
  bool string(json& data, const char *regex);

  bool integer(json& data, int min = INT_MIN, int max = INT_MAX);
  bool decimal(json& data, double min = -__DBL_MAX__, double max = __DBL_MAX__);
  bool boolean(json& data);

  bool steamid(json& data);
  bool base64(json& data);
}

#endif