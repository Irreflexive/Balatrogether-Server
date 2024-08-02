#ifndef BALATROGETHER_CONSOLE_H
#define BALATROGETHER_CONSOLE_H

#include "server.hpp"

struct Console;

class Command {
  public:
    Command(string name, std::vector<string> params, string desc, int num_optional = 0);
    string getUsage();
    virtual void execute(Console *console, std::vector<string> args) {};
    friend class Console;
  private:
    string name;
    std::vector<string> params;
    int num_optional;
    string desc;
};

struct Console {
  Console(Server *server);
  ~Console();
  bool process(Command *command, string input);
  void execute(string cmd, std::vector<string> args);
  Server *server;
  std::vector<Command*> commands;
};

void console_thread(Console *console);

#endif