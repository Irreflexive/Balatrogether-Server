#include "util/logs.hpp"
#include "console.hpp"

using namespace balatrogether;

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
    if (i >= this->params.size() - this->num_optional) {
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
    void execute(console_t console, std::unordered_map<string, string> args) {
      logger::info << "Command list:" << std::endl;
      for (command_t command : console->commands) {
        logger::info << command->getUsage() << std::endl;
      }
    };
};

class PlayerListCommand : public Command {
  public:
    PlayerListCommand() : Command("list", {"lobby"}, "Display a list of all connected players", 1) {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      client_list_t clients = console->server->getClients();
      if (args.size() >= 1) {
        int lobbyNumber = strtol(args["lobby"].c_str(), nullptr, 10);
        lobby_t lobby = console->server->getLobby(lobbyNumber);
        if (lobby) {
          clients = lobby->getClients();
          logger::info << clients.size() << " player(s) connected to room " << lobbyNumber << std::endl;
        } else {
          logger::info << clients.size() << " player(s) connected to server" << std::endl;
        }
      } else {
        logger::info << clients.size() << " player(s) connected to server" << std::endl;
      }
      for (client_t c : clients) {
        if (!c->getPlayer()) continue;
        logger::info << c->getPlayer()->getSteamId() << std::endl;
      }
    };
};

class StopCommand : public Command {
  public:
    StopCommand() : Command("stop", {"lobby"}, "Immediately shutdown the server/lobby", 1) {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      lobby_t lobby = nullptr;
      if (args.size() >= 1) {
        int lobbyNumber = strtol(args["lobby"].c_str(), nullptr, 10);
        lobby = console->server->getLobby(lobbyNumber);
      }
      if (lobby) {
        logger::info << "Stopping room " << lobby->getRoomNumber() << std::endl;
        lobby->close();
      } else {
        delete console->server;
        delete console;
        exit(0);
      }
    };
};

class LobbyListCommand : public Command {
  public:
    LobbyListCommand() : Command("lobbies", {"page"}, "List information about the server's lobbies", 1) {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      lobby_list_t lobbies = console->server->getLobbies();
      size_t page = 1;
      if (args.size() >= 1) {
        page = strtol(args["page"].c_str(), nullptr, 0);
      }
      size_t max_pages = ceil(lobbies.size() / 4.0);
      if (page < 1 || page > max_pages) {
        logger::info << "Usage: " << this->getUsage() << std::endl;
      } else {
        size_t start = (page - 1)*4 + 1;
        size_t end = std::min(page*4, lobbies.size());
        logger::info << "Showing rooms " << start << "-" << end << " of " << lobbies.size() << std::endl;
        for (int i = start; i <= end; i++) {
          lobby_t lobby = console->server->getLobby(i);
          logger::info << "Room " << lobby->getRoomNumber() << " - " << lobby->getClients().size() << "/" << console->server->getConfig()->getMaxPlayers() << std::endl;
        }
      }
    };
};

class KickCommand : public Command {
  public:
    KickCommand() : Command("kick", {"id"}, "Disconnects a player from the server by their Steam ID") {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      for (client_t c : console->server->getClients()) {
        if (!c->getPlayer()) continue;
        if (args["id"] == c->getPlayer()->getSteamId()) {
          console->server->disconnect(c);
          logger::info << "Player " << args["id"] << " kicked" << std::endl;
        }
      }
    };
};

class BanCommand : public Command {
  public:
    BanCommand() : Command("ban", {"id"}, "Disconnects a player and bans by their Steam ID") {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      console->execute("kick", args);
      console->server->getConfig()->ban(args["id"]);
        logger::info << "Player " << args["id"] << " banned" << std::endl;
    };
};

class UnbanCommand : public Command {
  public:
    UnbanCommand() : Command("unban", {"id"}, "Remove a Steam ID from the ban list") {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      console->server->getConfig()->unban(args["id"]);
        logger::info << "Player " << args["id"] << " unbanned" << std::endl;
    };
};

class WhitelistCommand : public Command {
  public:
    WhitelistCommand() : Command("whitelist", {"on/off/add/remove", "id"}, "Manages the server whitelist", 1) {};
    void execute(console_t console, std::unordered_map<string, string> args) {
      string subcommand = args["on/off/add/remove"];
      if (subcommand == "on") {
        console->server->getConfig()->setWhitelistEnabled(true);
        logger::info << "Whitelist enabled" << std::endl;
      } else if (subcommand == "off") {
        console->server->getConfig()->setWhitelistEnabled(false);
        logger::info << "Whitelist disabled" << std::endl;
      } else if (subcommand == "add" && args.size() >= 2) {
        steamid_t steamId = args["id"];
        console->server->getConfig()->whitelist(steamId);
        logger::info << "Added player " << args["id"] << " to whitelist" << std::endl;
      } else if (subcommand == "remove" && args.size() >= 2) {
        steamid_t steamId = args["id"];
        console->server->getConfig()->unwhitelist(steamId);
        logger::info << "Removed player " << args["id"] << " from whitelist" << std::endl;
      } else {
        logger::info << "Usage: " << this->getUsage() << std::endl;
      }
    };
};

Console::Console(server_t server)
{
  this->server = server;
  this->commands.push_back(new HelpCommand);
  this->commands.push_back(new PlayerListCommand);
  this->commands.push_back(new LobbyListCommand);
  this->commands.push_back(new StopCommand);
  this->commands.push_back(new KickCommand);
  this->commands.push_back(new BanCommand);
  this->commands.push_back(new UnbanCommand);
  this->commands.push_back(new WhitelistCommand);
  std::sort(this->commands.begin(), this->commands.end(), [](command_t a, command_t b) {
    return a->getUsage() < b->getUsage();
  });
}

Console::~Console()
{
  for (command_t command : this->commands) {
    delete command;
  }
}

bool Console::process(command_t command, string input)
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
    logger::info << "Usage: " << command->getUsage() << std::endl;
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
  for (command_t command : this->commands) {
    if (command->name == cmd) {
      command->execute(this, args);
      return;
    }
  }
}

void balatrogether::console_thread(console_t console)
{
  while (true) {
    string line;
    std::getline(std::cin, line);
    console->server->lock();
    bool matchedCommand = false;
    logger::info << "Console executed command \"" << line << "\"" << std::endl;
    for (command_t command : console->commands) {
      if (console->process(command, line)) {
        matchedCommand = true;
        break;
      }
    }
    if (!matchedCommand) {
      logger::info << "Unknown command, type \"help\" for a list of commands" << std::endl;
    }
    console->server->unlock();
  }
}