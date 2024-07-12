#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

void send_to_server(int sockfd, char* buffer, json payload) {
  payload["steam_id"] = "0";
  memset(buffer, 0, 4096);
  strcpy(buffer, payload.dump().c_str());
  send(sockfd, buffer, 4096, 0);
}

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(7063);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  char buffer[4096];
  connect(sockfd, (const sockaddr*) &serv_addr, sizeof(serv_addr));
  json payload;
  payload["cmd"] = "JOIN";
  send_to_server(sockfd, buffer, payload);

  int in;

  while (1) {
    std::cin >> in;
    json req;
    if (in == 1) {
      close(sockfd);
      break;
    } else if (in == 2) {
      req["cmd"] = "HIGHLIGHT";
      req["type"] = "hand";
      req["index"] = 2;
      send_to_server(sockfd, buffer, req);
    } else if (in == 3) {
      req["cmd"] = "HIGHLIGHT";
      req["type"] = "jokers";
      req["index"] = 1;
      send_to_server(sockfd, buffer, req);
    } else if (in == 4) {
      req["cmd"] = "HIGHLIGHT";
      req["type"] = "consumeables";
      req["index"] = 1;
      send_to_server(sockfd, buffer, req);
    }
  }
}