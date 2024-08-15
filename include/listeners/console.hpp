#ifndef BALATROGETHER_CONSOLE_LISTENER_HPP
#define BALATROGETHER_CONSOLE_LISTENER_HPP

#include "types.hpp"
#include "listener.hpp"
#include "events/console.hpp"

namespace balatrogether {
  class ConsoleEventListener : public EventListener<server_t, ConsoleEvent*> {
    public:
      ConsoleEventListener(server_t server);
      std::vector<ConsoleEvent*> getEvents();
      bool processCommand(ConsoleEvent* command, string input);
      bool process(string line);
      void client_error(server_t server, client_t client, json req, client_exception& e);
  };
}

#endif