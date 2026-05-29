#ifndef ASCII_DUNGEON_TYPES_H
#define ASCII_DUNGEON_TYPES_H

#include <map>
#include <string>
#include <vector>

// 二维坐标，x 表示列，y 表示行；整个工程都用这个约定。
struct Point {
  int x = 0;
  int y = 0;

  Point() = default;
  Point(int xValue, int yValue) : x(xValue), y(yValue) {}

  bool operator==(const Point &other) const {
    return x == other.x && y == other.y;
  }

  bool operator!=(const Point &other) const { return !(*this == other); }
};

inline Point operator+(const Point &left, const Point &right) {
  return {left.x + right.x, left.y + right.y};
}

inline Point operator-(const Point &left, const Point &right) {
  return {left.x - right.x, left.y - right.y};
}

inline int signOf(int value) {
  if (value > 0) {
    return 1;
  }
  if (value < 0) {
    return -1;
  }
  return 0;
}

enum class ScreenState { MainMenu, Playing, GameOver };

enum class MonsterKind { Goblin, Skeleton, Orc, Troll, Dragon };

enum class PickupKind { Health, Ammo };

struct Pickup {
  Point pos;
  PickupKind kind = PickupKind::Health;
};

struct Player {
  Point pos;
  int hp = 100;
  int maxHp = 100;
  int ammo = 28;
  int maxAmmo = 36;
};

struct Monster {
  MonsterKind kind = MonsterKind::Goblin;
  Point pos;
  int hp = 20;
  int maxHp = 20;
  int damage = 5;
  int followRange = 8;
  int moveDelay = 8;
  int moveTimer = 0;
  bool alive = true;
};

struct Bullet {
  Point pos;
  std::vector<Point> path;
  std::size_t nextIndex = 0;
  int damage = 20;
  bool alive = true;
};

struct Achievements {
  int levelsCleared = 0;
  int flawlessClears = 0;
  std::map<MonsterKind, int> killsByKind;
};

std::string monsterName(MonsterKind kind);
char monsterGlyph(MonsterKind kind);

#endif
