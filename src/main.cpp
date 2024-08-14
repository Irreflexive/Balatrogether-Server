#include <thread>
#include <csignal>
#include "util/logs.hpp"
#include "server.hpp"
#include "console.hpp"

#define PORT 7063

balatrogether::console_t console;

int main() {
  balatrogether::server_t server = new balatrogether::Server(PORT);
  balatrogether::logger::info << "Balatrogether is listening on port " << PORT << std::endl;

  console = new balatrogether::Console(server);
  std::thread(balatrogether::console_thread, console).detach();

  auto stopFunc = [](int s) { console->execute("stop", {}); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    server->acceptClient();
  }
}