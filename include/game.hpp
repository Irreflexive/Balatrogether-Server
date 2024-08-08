#ifndef BALATROGETHER_GAME_H
#define BALATROGETHER_GAME_H

#include "player.hpp"

typedef std::pair<player_t, double> score_t;
typedef std::vector<score_t> leaderboard_t;

struct Game {
  bool inGame;
  bool versus;
  bool bossPhase;
  player_list_t ready;
  player_list_t eliminated;
  leaderboard_t scores;
};

#endif