#ifndef BALATROGETHER_GAME_H
#define BALATROGETHER_GAME_H

#include <unordered_set>
#include "types.hpp"
#include "player.hpp"

namespace balatrogether {
  enum GameState : int {
    NOT_RUNNING = 1,
    IN_PROGRESS,
    WAITING_FOR_BOSS,
    FIGHTING_BOSS,
    WAITING_FOR_LEADERBOARD,
  };

  class Game {
    typedef std::pair<player_t, double> score_t;
    typedef std::vector<score_t> leaderboard_t;
    public:
      Game();

      bool isRunning();
      bool isVersus();
      bool isCoop();
      void reset();
      void start(player_list_t players, bool versus);

      player_list_t getPlayers();
      player_list_t getRemaining();
      player_list_t getEliminated();
      player_t getRandomPlayer();
      player_t getRandomPlayer(player_t exclude);

      game_state_t getState();
      void setState(game_state_t state);
      bool isEliminated(player_t p);
      void eliminate(player_t p);
      bool isBossReady();
      void prepareForBoss(player_t p);
      bool isScoringFinished();
      json getLeaderboard();
      void addScore(player_t p, double score);

      json getJSON();
    private:
      game_state_t state;
      bool versus;
      player_list_t players;
      std::unordered_set<player_t> readyForBoss;
      std::unordered_set<player_t> eliminated;
      leaderboard_t scores;
  };
}

#endif