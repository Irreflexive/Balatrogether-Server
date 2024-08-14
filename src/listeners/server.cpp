#include "listeners/server.hpp"
#include "events/setup.hpp"

ServerEventListener::ServerEventListener(server_t server) : EventListener(server)
{
  this->add(new JoinEvent);
  this->add(new JoinLobbyEvent);

  logger::info << "Listening for client events" << std::endl;
}

void ServerEventListener::client_error(server_t server, client_t client, json req, client_exception &e)
{
  server->getNetworkManager()->send({client}, error(e.what()));
}