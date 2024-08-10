#include "util.hpp"
#include "console.hpp"

Command::Command(string name, std::vector<string> params, string desc, int num_optional)
{
  this->name = name;
  this->params = params;
  this->desc = desc;
  this->num_optional = num_optional;
}

string Command::getUsage()
{
  string usage = this->name;
  for (int i = 0; i < this->params.size(); i++) {
    if (i > this->params.size() - 1 - num_optional) {
      usage += " [" + this->params[i] + "]";
    } else {
      usage += " <" + this->params[i] + ">";
    }
  }
  usage += " - " + this->desc;
  return usage;
}

class HelpCommand : public Command {
  public:
    HelpCommand() : Command("help", {}, "Displays this list of commands") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      logger::info("Command list:");
      for (Command *command : console->commands) {
        logger::info(command->getUsage());
      }
    };
};

class PlayerListCommand : public Command {
  public:
    PlayerListCommand() : Command("list", {}, "Display a list of all connected players") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      logger::info("%d player(s) connected", console->server->getClients().size());
      for (client_t c : console->server->getClients()) {
        logger::info(c->getPlayer()->getSteamId());
      }
    };
};

class StopCommand : public Command {
  public:
    StopCommand() : Command("stop", {}, "Immediately shutdown the server") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      console->server->stop();
      delete console->server;
      delete console;
      exit(0);
    };
};

class KickCommand : public Command {
  public:
    KickCommand() : Command("kick", {"id"}, "Disconnects a player from the server by their Steam ID") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      for (client_t c : console->server->getClients()) {
        if (args["id"] == c->getPlayer()->getSteamId()) {
          console->server->disconnect(c);
          logger::info("Player %s kicked", args["id"].c_str());
        }
      }
    };
};

class BanCommand : public Command {
  public:
    BanCommand() : Command("ban", {"id"}, "Disconnects a player and bans by their Steam ID") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      console->execute("kick", args);
      console->server->getConfig()->ban(args["id"]);
      logger::info("Player %s banned", args["id"].c_str());
    };
};

class UnbanCommand : public Command {
  public:
    UnbanCommand() : Command("unban", {"id"}, "Remove a Steam ID from the ban list") {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      console->server->getConfig()->unban(args["id"]);
      logger::info("Player %s unbanned", args["id"].c_str());
    };
};

class WhitelistCommand : public Command {
  public:
    WhitelistCommand() : Command("whitelist", {"on/off/add/remove", "id"}, "Manages the server whitelist", 1) {};
    void execute(Console *console, std::unordered_map<string, string> args) {
      string subcommand = args["on/off/add/remove"];
      if (subcommand == "on") {
        console->server->getConfig()->setWhitelistEnabled(true);
        logger::info("Whitelist enabled");
      } else if (subcommand == "off") {
        console->server->getConfig()->setWhitelistEnabled(false);
        logger::info("Whitelist disabled");
      } else if (subcommand == "add" && args.size() >= 2) {
        steamid_t steamId = args["id"];
        console->server->getConfig()->whitelist(steamId);
        logger::info("Added player %s to whitelist", steamId.c_str());
      } else if (subcommand == "remove" && args.size() >= 2) {
        steamid_t steamId = args["id"];
        console->server->getConfig()->unwhitelist(steamId);
        logger::info("Removed player %s from whitelist", steamId.c_str());
      } else {
        logger::info("Usage: %s", this->getUsage().c_str());
      }
    };
};

Console::Console(server_t server)
{
  this->server = server;
  this->commands.push_back(new HelpCommand);
  this->commands.push_back(new PlayerListCommand);
  this->commands.push_back(new StopCommand);
  this->commands.push_back(new KickCommand);
  this->commands.push_back(new BanCommand);
  this->commands.push_back(new UnbanCommand);
  this->commands.push_back(new WhitelistCommand);
}

Console::~Console()
{
  for (Command *command : this->commands) {
    delete command;
  }
}

bool Console::process(Command *command, string input)
{
  std::vector<string> args;
  while (true) {
    size_t n = input.find(' ');
    if (n == string::npos) {
      args.push_back(input);
      break;
    } else {
      args.push_back(input.substr(0, n));
      input = input.substr(n + 1);
    }
  }
  string cmd = args.front();
  if (cmd != command->name) return false;
  args.erase(args.begin());
  if (args.size() < command->params.size() - command->num_optional) {
    logger::info("Usage: %s", command->getUsage().c_str());
    return true;
  }
  if (args.size() > command->params.size() && command->params.size() > 0) {
    for (int i = args.size() - 1; i >= command->params.size(); i--) {
      string arg = args.back();
      args.pop_back();
      args[args.size() - 1] += " " + arg;
    }
  }
  std::unordered_map<string, string> argMap;
  for (int i = 0; i < args.size() && i < command->params.size(); i++) {
    argMap[command->params.at(i)] = args.at(i);
  }
  this->execute(command->name, argMap);
  return true;
}

void Console::execute(string cmd, std::unordered_map<string, string> args)
{
  for (Command *command : this->commands) {
    if (command->name == cmd) {
      command->execute(this, args);
      return;
    }
  }
}

void console_thread(Console *console)
{
  while (true) {
    string line;
    std::getline(std::cin, line);
    console->server->lock();
    bool matchedCommand = false;
    logger::info("Console executed command \"%s\"", line.c_str());
    for (Command *command : console->commands) {
      if (console->process(command, line)) {
        matchedCommand = true;
        break;
      }
    }
    if (!matchedCommand) {
      logger::info("Unknown command, type \"help\" for a list of commands");
    }
    console->server->unlock();
  }
}