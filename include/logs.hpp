#ifndef BALATROGETHER_LOGS_H
#define BALATROGETHER_LOGS_H

#include <string>

using std::string;

namespace logger {
  void setDebugOutputEnabled(bool enabled);

  int infoLog(string format, ...);
  int debugLog(string format, ...);
  int errorLog(string format, ...);
}

#endif