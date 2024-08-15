#include "listeners/console.hpp"

using namespace balatrogether;

ConsoleEventListener::ConsoleEventListener(server_t server) : EventListener(server)
{
  this->add(new HelpCommand);
  this->add(new PlayerListCommand);
  this->add(new StopCommand);
  this->add(new LobbyListCommand);
  this->add(new KickCommand);
  this->add(new BanCommand);
  this->add(new UnbanCommand);
  this->add(new WhitelistCommand);

  logger::info << "Console commands registered" << std::endl;
}

std::vector<ConsoleEvent*> ConsoleEventListener::getEvents()
{
  return this->events;
}

bool ConsoleEventListener::processCommand(ConsoleEvent *command, string input)
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
  if (cmd != command->getCommand()) return false;
  args.erase(args.begin());
  std::vector<string> params = command->getParameters();
  if (args.size() < params.size() - command->getOptionalParameterCount()) {
    logger::info << "Usage: " << command->getUsage() << std::endl;
    return true;
  }
  if (args.size() > params.size() && params.size() > 0) {
    for (int i = args.size() - 1; i >= params.size(); i--) {
      string arg = args.back();
      args.pop_back();
      args[args.size() - 1] += " " + arg;
    }
  }
  json argMap;
  for (int i = 0; i < args.size() && i < params.size(); i++) {
    argMap[params.at(i)] = args.at(i);
  }
  command->execute(this->object, nullptr, argMap);
  return true;
}

bool ConsoleEventListener::process(string line) 
{
  bool matchedCommand = false;
  logger::info << "Console executed command \"" << line << "\"" << std::endl;
  for (ConsoleEvent* event : this->object->getConsole()->getEvents()) {
    if (this->processCommand(event, line)) {
      matchedCommand = true;
      break;
    }
  }
  if (!matchedCommand) logger::info << "Unknown command, type \"help\" for a list of commands" << std::endl;
  return false;
}

void ConsoleEventListener::client_error(server_t server, client_t client, json req, client_exception& e) {
  return;
}