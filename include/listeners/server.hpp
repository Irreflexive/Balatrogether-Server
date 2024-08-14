#ifndef BALATROGETHER_SERVER_LISTENER_HPP
#define BALATROGETHER_SERVER_LISTENER_HPP

#include "types.hpp"
#include "listener.hpp"

namespace balatrogether {
  class ServerEventListener : public EventListener<server_t> {
    public:
      ServerEventListener(server_t server);
      void client_error(server_t server, client_t client, json req, client_exception& e);
  };
}

#endif