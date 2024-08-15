#include <thread>
#include <csignal>
#include "util/logs.hpp"
#include "server.hpp"

#define PORT 7063

balatrogether::server_t server;

void resetColors() {
  std::cout << "\033[0m";
  std::cerr << "\033[0m";
}

int main() {
  server = new balatrogether::Server(PORT);
  balatrogether::logger::info << "Balatrogether is listening on port " << PORT << std::endl;

  auto atAbort = [](int s) { resetColors(); };
  atexit(resetColors);
  signal(SIGABRT, atAbort);

  auto stopFunc = [](int s) { delete server; exit(0); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    server->acceptClient();
  }
}