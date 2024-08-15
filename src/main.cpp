#include <thread>
#include <csignal>
#include "util/logs.hpp"
#include "server.hpp"

#define PORT 7063

balatrogether::server_t server;

int main() {
  server = new balatrogether::Server(PORT);
  balatrogether::logger::info << "Balatrogether is listening on port " << PORT << std::endl;

  auto stopFunc = [](int s) { delete server; exit(0); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    server->acceptClient();
  }
}