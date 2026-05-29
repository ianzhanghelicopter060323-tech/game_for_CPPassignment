#ifndef ASCII_DUNGEON_GAME_H
#define ASCII_DUNGEON_GAME_H

#include "Maze.h"
#include "Types.h"

#include <optional>
#include <random>
#include <string>
#include <vector>

class Game {
public:
  Game();

  void handleKey(int ch);
  void handleMouseClick(Point worldPoint);
  void update();
  void startNewGame();

  bool wantsQuit() const { return quitRequested; }
  ScreenState screen() const { return screenState; }
  int selectedMenuIndex() const { return selectedMenu; }

  const Maze &getMaze() const { return maze; }
  const Player &getPlayer() const { return player; }
  const std::vector<Monster> &getMonsters() const { return monsters; }
  const std::vector<Pickup> &getPickups() const { return pickups; }
  const std::vector<Bullet> &getBullets() const { return bullets; }
  const Achievements &getAchievements() const { return achievements; }

  int level() const { return currentLevel; }
  int sprintCooldownTicks() const { return sprintCooldown; }
  int sprintCooldownMaxTicks() const { return sprintCooldownMax; }
  const std::string &message() const { return statusMessage; }

private:
  Maze maze;
  ScreenState screenState = ScreenState::MainMenu;
  Player player;
  std::vector<Monster> monsters;
  std::vector<Pickup> pickups;
  std::vector<Bullet> bullets;
  Achievements achievements;
  std::mt19937 rng;

  int currentLevel = 1;
  int selectedMenu = 0;
  int frame = 0;
  int sprintCooldown = 0;
  const int sprintCooldownMax = 58;
  bool quitRequested = false;
  bool levelTookDamage = false;
  std::string statusMessage = "准备进入地牢。";
  int messageTimer = 0;

  void buildLevel();
  void spawnMonsters();
  void spawnPickups();
  void movePlayer(Point delta, bool sprint);
  void shootAt(Point target);
  void updateBullets();
  void updateMonsters();
  void finishLevel();
  void damagePlayer(int amount, const std::string &reason);
  void setMessage(const std::string &message, int frames = 95);

  std::optional<std::size_t> monsterAt(Point p) const;
  std::optional<std::size_t> pickupAt(Point p) const;
  bool occupiedByActor(Point p) const;
  Monster makeMonster(MonsterKind kind) const;
  Point chooseMonsterStep(const Monster &monster);
  Point nextStepByBfs(Point from, Point to) const;
  std::vector<Point> buildBulletPath(Point from, Point to) const;
};

#endif
