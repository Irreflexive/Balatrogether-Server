#ifndef BALATROGETHER_CONSOLE_EVENTS_H
#define BALATROGETHER_CONSOLE_EVENTS_H

#include "server.hpp"
#include "listener.hpp"

namespace balatrogether {
  class ConsoleEvent : public Event<server_t> {
    public:
      ConsoleEvent(string name, std::vector<string> params, string desc, int num_optional = 0);
      std::vector<string> getParameters();
      string getDescription();
      int getOptionalParameterCount();
      string getUsage();
      virtual void execute(server_t server, client_t client, json args) {};
    private:
      std::vector<string> params;
      int num_optional;
      string desc;
  };

  class HelpCommand : public ConsoleEvent {
    public:
      HelpCommand() : ConsoleEvent("help", {}, "Displays this list of commands") {};
      void execute(server_t server, client_t client, json args);
  };

  class PlayerListCommand : public ConsoleEvent {
    public:
      PlayerListCommand() : ConsoleEvent("list", {"lobby"}, "Display a list of all connected players", 1) {};
      void execute(server_t server, client_t client, json args);
  };

  class StopCommand : public ConsoleEvent {
    public:
      StopCommand() : ConsoleEvent("stop", {"lobby"}, "Immediately shutdown the server/lobby", 1) {};
      void execute(server_t server, client_t client, json args);
  };

  class LobbyListCommand : public ConsoleEvent {
    public:
      LobbyListCommand() : ConsoleEvent("lobbies", {"page"}, "List information about the server's lobbies", 1) {};
      void execute(server_t server, client_t client, json args);
  };

  class KickCommand : public ConsoleEvent {
    public:
      KickCommand() : ConsoleEvent("kick", {"id"}, "Disconnects a player from the server by their Steam ID") {};
      void execute(server_t server, client_t client, json args);
  };

  class BanCommand : public ConsoleEvent {
    public:
      BanCommand() : ConsoleEvent("ban", {"id"}, "Disconnects a player and bans by their Steam ID") {};
      void execute(server_t server, client_t client, json args);
  };

  class UnbanCommand : public ConsoleEvent {
    public:
      UnbanCommand() : ConsoleEvent("unban", {"id"}, "Remove a Steam ID from the ban list") {};
      void execute(server_t server, client_t client, json args);
  };

  class WhitelistCommand : public ConsoleEvent {
    public:
      WhitelistCommand() : ConsoleEvent("whitelist", {"on/off/add/remove", "id"}, "Manages the server whitelist", 1) {};
      void execute(server_t server, client_t client, json args);
  };
}

#endif