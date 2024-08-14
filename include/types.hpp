#ifndef BALATROGETHER_TYPES_H
#define BALATROGETHER_TYPES_H

#include <memory>
#include <string>
#include "json.hpp"

namespace balatrogether {
  using json = nlohmann::json;
  using string = std::string;
  typedef std::string steamid_t;

  // console.hpp
  struct Console;
  class Command;
  typedef Console* console_t;
  typedef Command* command_t;

  // server.hpp
  class Server;
  typedef Server* server_t;

  // config.hpp
  class Config;
  typedef Config* config_t;

  // lobby.hpp
  class Lobby;
  typedef Lobby* lobby_t;
  typedef std::vector<lobby_t> lobby_list_t;

  // game.hpp
  class Game;
  enum GameState : int;
  typedef Game* game_t;
  typedef enum GameState game_state_t;

  // preq.hpp
  class PersistentRequest;
  class PersistentRequestManager;
  typedef PersistentRequest* preq_t;
  typedef PersistentRequestManager* preq_manager_t;

  // client.hpp
  class Client;
  typedef Client* client_t;
  typedef std::vector<client_t> client_list_t;

  // player.hpp
  class Player;
  typedef std::shared_ptr<Player> player_t;
  typedef std::vector<player_t> player_list_t;

  // network.hpp
  class NetworkManager;
  typedef NetworkManager* network_t;

  // listener.hpp
  template <class T> class Event;
  template <class T> class EventListener;
  class ServerEventListener;
  class LobbyEventListener;
  typedef ServerEventListener* server_listener_t;
  typedef LobbyEventListener* lobby_listener_t;

  // util.hpp
  class client_exception;
}

#endif