#ifndef BALATROGETHER_GAME_H
#define BALATROGETHER_GAME_H

#include "player.hpp"

struct Game {
  bool inGame;
  bool versus;
  bool bossPhase;
  steamid_list_t ready;
  steamid_list_t eliminated;
  leaderboard_t scores;
};

#endif