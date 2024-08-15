#include "events/console.hpp"
#include "lobby.hpp"
#include "client.hpp"

using namespace balatrogether;

ConsoleEvent::ConsoleEvent(string name, std::vector<string> params, string desc, int num_optional) : Event(name) {
  this->params = params;
  this->desc = desc;
  this->num_optional = num_optional;
}

std::vector<string> ConsoleEvent::getParameters()
{
  return this->params;
}

string ConsoleEvent::getDescription()
{
  return this->desc;
}

int ConsoleEvent::getOptionalParameterCount()
{
  return this->num_optional;
}

string ConsoleEvent::getUsage()
{
  string usage = this->getCommand();
  std::vector<string> params = this->getParameters();
  for (int i = 0; i < params.size(); i++) {
    if (i >= params.size() - this->getOptionalParameterCount()) {
      usage += " [" + params[i] + "]";
    } else {
      usage += " <" + params[i] + ">";
    }
  }
  usage += " - " + this->getDescription();
  return usage;
}

void HelpCommand::execute(server_t server, client_t client, json args)
{
  logger::info << "Command list:" << std::endl;
  for (ConsoleEvent* event : server->getConsole()->getEvents()) {
    logger::info << event->getUsage() << std::endl;
  }
}

void PlayerListCommand::execute(server_t server, client_t client, json args)
{
  client_list_t clients = server->getClients();
  if (args.size() >= 1) {
    int lobbyNumber = strtol(args["lobby"].get<string>().c_str(), nullptr, 10);
    lobby_t lobby = server->getLobby(lobbyNumber);
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
}

void StopCommand::execute(server_t server, client_t client, json args)
{
  lobby_t lobby = nullptr;
  if (args.size() >= 1) {
    int lobbyNumber = strtol(args["lobby"].get<string>().c_str(), nullptr, 10);
    lobby = server->getLobby(lobbyNumber);
  }
  if (lobby) {
    logger::info << "Stopping room " << lobby->getRoomNumber() << std::endl;
    lobby->close();
  } else {
    delete server;
    exit(0);
  }
}

void LobbyListCommand::execute(server_t server, client_t client, json args)
{
  lobby_list_t lobbies = server->getLobbies();
  size_t page = 1;
  if (args.size() >= 1) {
    page = strtol(args["page"].get<string>().c_str(), nullptr, 0);
  }
  size_t max_pages = ceil(lobbies.size() / 4.0);
  if (page < 1 || page > max_pages) {
    logger::info << "Usage: " << this->getUsage() << std::endl;
  } else {
    size_t start = (page - 1)*4 + 1;
    size_t end = std::min(page*4, lobbies.size());
    logger::info << "Showing rooms " << start << "-" << end << " of " << lobbies.size() << std::endl;
    for (int i = start; i <= end; i++) {
      lobby_t lobby = server->getLobby(i);
      logger::info << "Room " << lobby->getRoomNumber() << " - " << lobby->getClients().size() << "/" << server->getConfig()->getMaxPlayers() << std::endl;
    }
  }
}

void KickCommand::execute(server_t server, client_t client, json args)
{
  for (client_t c : server->getClients()) {
    if (!c->getPlayer()) continue;
    if (args["id"].get<steamid_t>() == c->getPlayer()->getSteamId()) {
      server->disconnect(c);
      logger::info << "Player " << args["id"].get<steamid_t>() << " kicked" << std::endl;
    }
  }
}

void BanCommand::execute(server_t server, client_t client, json args)
{
  for (ConsoleEvent* event : server->getConsole()->getEvents()) {
    if (event->getCommand() == "kick") {
      event->execute(server, client, args);
    }
  }
  server->getConfig()->ban(args["id"].get<steamid_t>());
  logger::info << "Player " << args["id"].get<steamid_t>() << " banned" << std::endl;
}

void UnbanCommand::execute(server_t server, client_t client, json args)
{
  server->getConfig()->unban(args["id"].get<steamid_t>());
  logger::info << "Player " << args["id"].get<steamid_t>() << " unbanned" << std::endl;
}

void WhitelistCommand::execute(server_t server, client_t client, json args)
{
  string subcommand = args["on/off/add/remove"].get<string>();
  if (subcommand == "on") {
    server->getConfig()->setWhitelistEnabled(true);
    logger::info << "Whitelist enabled" << std::endl;
  } else if (subcommand == "off") {
    server->getConfig()->setWhitelistEnabled(false);
    logger::info << "Whitelist disabled" << std::endl;
  } else if (subcommand == "add" && args.size() >= 2) {
    steamid_t steamId = args["id"].get<steamid_t>();
    server->getConfig()->whitelist(steamId);
    logger::info << "Added player " << steamId << " to whitelist" << std::endl;
  } else if (subcommand == "remove" && args.size() >= 2) {
    steamid_t steamId = args["id"].get<steamid_t>();
    server->getConfig()->unwhitelist(steamId);
    logger::info << "Removed player " << steamId << " from whitelist" << std::endl;
  } else {
    logger::info << "Usage: " << this->getUsage() << std::endl;
  }
}
