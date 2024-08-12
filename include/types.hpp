#ifndef BALATROGETHER_TYPES_H
#define BALATROGETHER_TYPES_H

#include <memory>
#include <string>
#include "json.hpp"

namespace balatrogether {
  using json = nlohmann::json;
  using string = std::string;

  // console.hpp
  struct Console;
  class Command;

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
  typedef std::string steamid_t;
  typedef std::shared_ptr<Player> player_t;
  typedef std::vector<steamid_t> steamid_list_t;
  typedef std::vector<player_t> player_list_t;
  typedef std::pair<player_t, double> score_t;
  typedef std::vector<score_t> leaderboard_t;

  // network.hpp
  class NetworkManager;
  template <class T> class NetworkEvent;
  template <class T> class EventListener;
  typedef NetworkManager* network_t;
  typedef EventListener<server_t>* server_listener_t;
  typedef EventListener<lobby_t>* lobby_listener_t;

  // util.hpp
  class client_exception;
}

#endif