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
    HelpCommand() : Command("help", std::vector<string>(), "Displays this list of commands") {};
    void execute(Console *console, std::vector<string> args) {
      console->server->infoLog("Command list:");
      for (Command *command : console->commands) {
        console->server->infoLog(command->getUsage());
      }
    };
};

class PlayerListCommand : public Command {
  public:
    PlayerListCommand() : Command("list", std::vector<string>(), "Display a list of all connected players") {};
    void execute(Console *console, std::vector<string> args) {
      console->server->infoLog("%d player(s) connected", console->server->getPlayers().size());
      for (player_t p : console->server->getPlayers()) {
        console->server->infoLog(p->getSteamId());
      }
    };
};

class StopCommand : public Command {
  public:
    StopCommand() : Command("stop", std::vector<string>(), "Immediately shutdown the server") {};
    void execute(Console *console, std::vector<string> args) {
      console->server->stop();
      delete console;
      exit(0);
    };
};

class KickCommand : public Command {
  public:
    KickCommand() : Command("kick", {"id"}, "Disconnects a player from the server by their Steam ID") {};
    void execute(Console *console, std::vector<string> args) {
      steamid_t steamId = args.at(0);
      for (player_t p : console->server->getPlayers()) {
        if (steamId == p->getSteamId()) {
          console->server->disconnect(p);
          console->server->infoLog("Player %s kicked", steamId.c_str());
        }
      }
    };
};

class BanCommand : public Command {
  public:
    BanCommand() : Command("ban", {"id"}, "Disconnects a player and bans by their Steam ID") {};
    void execute(Console *console, std::vector<string> args) {
      console->execute("kick", args);
      steamid_t steamId = args.at(0);
      console->server->config.ban(steamId);
      console->server->infoLog("Player %s banned", steamId.c_str());
    };
};

class UnbanCommand : public Command {
  public:
    UnbanCommand() : Command("unban", {"id"}, "Remove a Steam ID from the ban list") {};
    void execute(Console *console, std::vector<string> args) {
      steamid_t steamId = args.at(0);
      console->server->config.unban(steamId);
      console->server->infoLog("Player %s unbanned", steamId.c_str());
    };
};

class WhitelistCommand : public Command {
  public:
    WhitelistCommand() : Command("whitelist", {"on/off/add/remove", "id"}, "Manages the server whitelist", 1) {};
    void execute(Console *console, std::vector<string> args) {
      string subcommand = args.at(0);
      if (subcommand == "on") {
        console->server->config.setWhitelistEnabled(true);
        console->server->infoLog("Whitelist enabled");
      } else if (subcommand == "off") {
        console->server->config.setWhitelistEnabled(false);
        console->server->infoLog("Whitelist disabled");
      } else if (subcommand == "add" && args.size() >= 2) {
        steamid_t steamId = args.at(1);
        console->server->config.whitelist(steamId);
        console->server->infoLog("Added player %s to whitelist", steamId.c_str());
      } else if (subcommand == "remove" && args.size() >= 2) {
        steamid_t steamId = args.at(1);
        console->server->config.unwhitelist(steamId);
        console->server->infoLog("Removed player %s from whitelist", steamId.c_str());
      } else {
        console->server->infoLog("Usage: %s", this->getUsage().c_str());
      }
    };
};

Console::Console(Server *server)
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
    this->server->infoLog("Usage: %s", command->getUsage().c_str());
    return true;
  }
  this->execute(command->name, args);
  return true;
}

void Console::execute(string cmd, std::vector<string> args)
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
    for (Command *command : console->commands) {
      if (console->process(command, line)) {
        matchedCommand = true;
        break;
      }
    }
    if (!matchedCommand) {
      console->server->infoLog("Unknown command, type \"help\" for a list of commands");
    }
    console->server->unlock();
  }
}