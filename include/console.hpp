#ifndef BALATROGETHER_CONSOLE_H
#define BALATROGETHER_CONSOLE_H

#include "types.hpp"
#include "server.hpp"

namespace balatrogether {
  class Command {
    public:
      Command(string name, std::vector<string> params, string desc, int num_optional = 0);
      string getUsage();
      virtual void execute(console_t console, std::unordered_map<string, string> args) {};
      friend class Console;
    private:
      string name;
      std::vector<string> params;
      int num_optional;
      string desc;
  };

  struct Console {
    Console(server_t server);
    ~Console();
    bool process(command_t command, string input);
    void execute(string cmd, std::unordered_map<string, string> args);
    server_t server;
    std::vector<command_t> commands;
  };

  void console_thread(console_t console);
}

#endif