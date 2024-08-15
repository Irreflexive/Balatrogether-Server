#ifndef BALATROGETHER_LOGS_UTIL_H
#define BALATROGETHER_LOGS_UTIL_H

#include <iostream>
#include <mutex>
#include "types.hpp"

namespace balatrogether::logger {
  extern std::mutex mutex;

  class stream {
    public:
      stream(string prefix = "", std::ostream& os = std::cout, string color = "0") : new_line(true), on(true), prefix(prefix), os(os), color(color) {};
      stream(const stream &other) : new_line(other.new_line), on(other.on), prefix(other.prefix), os(other.os), color(other.color) {};
      template<class T> stream &operator<<(T val) {
        if (!on) return *this;
        if (new_line) {
          mutex.lock();
          std::time_t t = std::time(nullptr);
          os << "\033[" << color << "m[" << std::put_time(std::gmtime(&t), "%FT%TZ") << "] " << prefix;
          new_line = false;
        }
        os << val;
        return *this;
      };
      stream &operator<<(std::ostream& (*fn)(std::ostream&)) {
        if (!on) return *this;
        bool is_endl = fn == (std::ostream& (*)(std::ostream&)) std::endl;
        bool is_flush = is_endl || fn == (std::ostream& (*)(std::ostream&)) std::flush;
        if (is_flush) os << "\033[34m";
        os << fn;
        if (is_endl) new_line = true;
        if (is_flush) mutex.unlock();
        return *this;
      };
      void setEnabled(bool enabled) { on = enabled; };
    private:
      bool new_line;
      bool on;
      string prefix;
      std::ostream& os;
      string color;
    };

  extern stream info;
  extern stream debug;
  extern stream error;
}

#endif