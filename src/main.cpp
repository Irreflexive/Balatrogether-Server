#include <csignal>
#include "util/logs.hpp"
#include "server.hpp"
#include "console.hpp"

#define PORT 7063

console_t console;

int main() {
  server_t server = new Server(PORT);
  logger::info << "Balatrogether is listening on port " << PORT << std::endl;

  console = new Console(server);
  std::thread(console_thread, console).detach();

  auto stopFunc = [](int s) { console->execute("stop", {}); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    server->acceptClient();
  }
}