#include <csignal>
#include "logs.hpp"
#include "server.hpp"
#include "console.hpp"

#define PORT 7063

Console* console;

int main() {
  Server* server = new Server(PORT);
  logger::infoLog("Balatrogether is listening on port %i", PORT);

  console = new Console(server);
  std::thread(console_thread, console).detach();

  auto stopFunc = [](int s) { console->execute("stop", {}); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    server->acceptClient();
  }
}