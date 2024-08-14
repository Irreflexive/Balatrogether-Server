#ifndef BALATROGETHER_UTIL_H
#define BALATROGETHER_UTIL_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string>
#include <iostream>
#include "types.hpp"

using namespace balatrogether;

namespace balatrogether {
  class client_exception : public std::runtime_error {
    public:
      client_exception(const char* msg, bool disconnect = false) : std::runtime_error(msg), disconnect(disconnect) {};
      bool keep() { return !this->disconnect; };
    private:
      bool disconnect;
  };

  string getpath();
  json success(string cmd, json data);
  json success(string cmd);
  json error(string msg);

  namespace logger {
    class stream {
      public:
        stream(string prefix = "", std::ostream& os = std::cout, string color = "0") : new_line(true), on(true), prefix(prefix), os(os), color(color) {};
        stream(const stream &other) : new_line(other.new_line), on(other.on), prefix(other.prefix), os(other.os), color(other.color) {};
        template<class T> stream &operator<<(T val) {
          if (!on) return *this;
          if(new_line) {
            std::time_t t = std::time(nullptr);
            os << "\033[" << color << "m[" << std::put_time(std::gmtime(&t), "%FT%TZ") << "] " << prefix;
            new_line = false;
          }
          os << val;
          return *this;
        };
        stream &operator<<(std::ostream& (*fn)(std::ostream&)) {
          if (!on) return *this;
          os << fn;
          if(fn == (std::ostream& (*)(std::ostream&)) std::endl) {
            os << "\033[0m";
            new_line = true;
          }
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

    void setDebugOutputEnabled(bool enabled);

    extern stream info;
    extern stream debug;
    extern stream error;
  }

  namespace ssl {
    SSL_CTX *create_context();
    void configure_context(SSL_CTX *ctx, bool debug);
  }

  namespace validation {
    bool string(json& data);
    bool string(json& data, size_t maxLength);
    bool string(json& data, size_t minLength, size_t maxLength);

    bool integer(json& data, int min = INT_MIN, int max = INT_MAX);
    bool decimal(json& data, double min = __DBL_MIN__, double max = __DBL_MAX__);
    bool boolean(json& data);

    bool steamid(json& data);
  }
}

#endif