#include "game.hpp"

using namespace balatrogether;

Game::Game()
{
  this->setState(NOT_RUNNING);
  this->versus = false;
}

bool Game::isRunning()
{
  return this->getState() != NOT_RUNNING;
}

bool Game::isVersus()
{
  return this->isRunning() && this->versus;
}

bool Game::isCoop()
{
  return this->isRunning() && !this->versus;
}

void Game::reset()
{
  this->setState(NOT_RUNNING);
  this->players.clear();
  this->readyForBoss.clear();
  this->eliminated.clear();
  this->scores.clear();
}

void Game::start(player_list_t players, bool versus)
{
  this->setState(IN_PROGRESS);
  this->versus = versus;
  this->players = players;
}

player_list_t Game::getPlayers()
{
  return this->players;
}

player_list_t Game::getRemaining()
{
  player_list_t remaining;
  for (player_t p : this->getPlayers()) {
    if (!this->isEliminated(p)) remaining.push_back(p);
  }
  return remaining;
}

player_list_t Game::getEliminated()
{
  player_list_t eliminated;
  for (player_t p : this->getPlayers()) {
    if (this->isEliminated(p)) eliminated.push_back(p);
  }
  return eliminated;
}

player_t Game::getRandomPlayer()
{
  player_list_t options = this->getRemaining();
  if (options.size() == 0) return nullptr;
  return options.at(rand() % options.size());
}

player_t Game::getRandomPlayer(player_t exclude)
{
  player_list_t options;
  for (player_t p : this->getRemaining()) {
    if (p->getSteamId() != exclude->getSteamId()) options.push_back(p);
  }
  if (options.size() == 0) return nullptr;
  return options.at(rand() % options.size());
}

game_state_t Game::getState()
{
  return this->state;
}

void Game::setState(game_state_t state)
{
  this->state = state;
}

bool Game::isEliminated(player_t p)
{
  return this->eliminated.find(p) != this->eliminated.end();
}

void Game::eliminate(player_t p)
{
  if (!this->isVersus()) return;
  if (this->isEliminated(p)) return;
  this->eliminated.insert(std::make_pair(p, true));
  for (int i = 0; i < this->scores.size(); i++) {
    score_t pair = this->scores[i];
    if (pair.first == p) {
      this->scores.erase(this->scores.begin() + i);
      break;
    }
  }
}

bool Game::isBossReady()
{
  for (player_t p : this->getRemaining()) {
    if (this->readyForBoss.find(p) == this->readyForBoss.end()) return false;
  }
  return true;
}

void Game::prepareForBoss(player_t p)
{
  if (!this->isVersus()) return;
  if (this->getState() != IN_PROGRESS && this->getState() != WAITING_FOR_BOSS) return;
  if (this->readyForBoss.find(p) != this->readyForBoss.end()) return;
  this->setState(WAITING_FOR_BOSS);
  this->readyForBoss.insert(std::make_pair(p, true));
}

bool Game::isScoringFinished()
{
  if (this->getState() != WAITING_FOR_LEADERBOARD) return false;
  for (player_t p : this->getRemaining()) {
    bool hasScored = false;
    for (score_t score : this->scores) {
      if (score.first == p) {
        hasScored = true;
        break;
      }
    }
    if (!hasScored) return false;
  }
  return true;
}

json Game::getLeaderboard()
{
  json leaderboard = json::array();
  for (score_t pair : this->scores) {
    json row;
    row["player"] = pair.first->getSteamId();
    row["score"] = pair.second;
    leaderboard.push_back(row);
  }
  player_list_t eliminatedPlayers = this->getEliminated();
  std::reverse(eliminatedPlayers.begin(), eliminatedPlayers.end());
  for (player_t eliminated : eliminatedPlayers) {
    json row;
    row["player"] = eliminated->getSteamId();
    leaderboard.push_back(row);
  }
  return leaderboard;
}

void Game::addScore(player_t p, double score)
{
  if (!this->isVersus()) return;
  if (this->getState() != FIGHTING_BOSS && this->getState() != WAITING_FOR_LEADERBOARD) return;
  for (score_t score : this->scores) {
    if (score.first == p) return;
  }
  this->setState(WAITING_FOR_LEADERBOARD);
  this->scores.push_back(score_t(p, score));
}

json Game::getJSON() 
{
  json data = {
    {"state", this->getState()},
    {"remaining", this->getRemaining().size()},
    {"eliminated", this->getEliminated().size()}
  };
  return data;
}