#ifndef BALATROGETHER_GAME_H
#define BALATROGETHER_GAME_H

#include "player.hpp"

typedef std::pair<steamid_t, double> score_t;
typedef std::vector<score_t> leaderboard_t;

struct Game {
  bool inGame;
  bool versus;
  bool bossPhase;
  steamid_list_t ready;
  steamid_list_t eliminated;
  leaderboard_t scores;
};

#endif