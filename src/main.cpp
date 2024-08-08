#include <csignal>
#include "server.hpp"
#include "console.hpp"

#define PORT 7063

Console* console;

int main() {
  Server* server = new Server();
  server->infoLog("Starting server");

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  server->infoLog("Configuring socket options");

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    server->errorLog("Failed to set reuse address");
    exit(1);
  }
  server->infoLog("Allowed address reuse");

  int send_size = BUFFER_SIZE;
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_size, sizeof(send_size)) < 0) {
    server->errorLog("Failed to set send buffer size");
    exit(1);
  }
  server->infoLog("Set send buffer size");

  int recv_size = BUFFER_SIZE;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_size, sizeof(recv_size)) < 0) {
    server->errorLog("Failed to set receive buffer size");
    exit(1);
  }
  server->infoLog("Set receive buffer size");

  int keepalive = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
    server->errorLog("Failed to set keepalive");
    exit(1);
  }
  server->infoLog("Enabled keepalive timer");

  int bindStatus = bind(sockfd, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
  if (bindStatus < 0) {
    server->errorLog("Failed to bind");
    exit(1);
  }
  server->infoLog("Bound to address");
  
  if (listen(sockfd, 3) < 0) {
    server->errorLog("Failed to listen");
    exit(1);
  }

  server->infoLog("Balatrogether is listening on port %i", PORT);
  console = new Console(server);
  std::thread(console_thread, console).detach();

  auto stopFunc = [](int s) { console->execute("stop", {}); };
  signal(SIGTERM, stopFunc);
  signal(SIGINT, stopFunc);

  while (true) {
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int clientfd = accept(sockfd, (struct sockaddr*) &cli_addr, &cli_len);
    player_t p = new Player(clientfd, cli_addr);
    server->infoLog("Client from %s attempting to connect", p->getIP().c_str());
    std::thread(client_thread, server, p).detach();
  }

  closesocket(sockfd);
}