#ifndef BALATROGETHER_GAME_H
#define BALATROGETHER_GAME_H

#include <unordered_map>
#include "json.hpp"
#include "player.hpp"

using json = nlohmann::json;

typedef std::pair<player_t, double> score_t;
typedef std::vector<score_t> leaderboard_t;

class Game {
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

    bool isEliminated(player_t p);
    void eliminate(player_t p);
    bool isBossReady();
    void prepareForBoss(player_t p);
    void setBossPhaseEnabled(bool enabled);
    bool isScoringFinished();
    json getLeaderboard();
    void addScore(player_t p, double score);
  private:
    bool inGame;
    bool versus;
    bool bossPhase;
    player_list_t players;
    std::unordered_map<player_t, bool> readyForBoss;
    std::unordered_map<player_t, bool> eliminated;
    leaderboard_t scores;
};

typedef Game* game_t;

#endif