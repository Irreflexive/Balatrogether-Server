#ifndef BALATROGETHER_LOGS_H
#define BALATROGETHER_LOGS_H

#include <string>

using std::string;

namespace logger {
  void setDebugOutputEnabled(bool enabled);

  int info(string format, ...);
  int debug(string format, ...);
  int error(string format, ...);
}

#endif