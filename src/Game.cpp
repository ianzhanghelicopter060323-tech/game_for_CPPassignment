#include "Game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <ncurses.h>
#include <queue>

namespace {
constexpr int kFramesPerSecond = 30;
constexpr int kPlayerInvincibleTicks = kFramesPerSecond;

int distanceManhattan(Point a, Point b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool sameLineOrDiagonal(Point a, Point b) {
  const int dx = std::abs(a.x - b.x);
  const int dy = std::abs(a.y - b.y);
  return dx == 0 || dy == 0 || dx == dy;
}
} // namespace

Game::Game()
    : maze(96, 32), rng(static_cast<unsigned int>(std::time(nullptr))) {
  achievements.killsByKind[MonsterKind::Goblin] = 0;
  achievements.killsByKind[MonsterKind::Skeleton] = 0;
  achievements.killsByKind[MonsterKind::Orc] = 0;
  achievements.killsByKind[MonsterKind::Troll] = 0;
  achievements.killsByKind[MonsterKind::Dragon] = 0;
}

void Game::handleKey(int ch) {
  if (screenState == ScreenState::MainMenu) {
    if (ch == 'w' || ch == 'W' || ch == KEY_UP) {
      selectedMenu = (selectedMenu + 1) % 2;
    } else if (ch == 's' || ch == 'S' || ch == KEY_DOWN) {
      selectedMenu = (selectedMenu + 1) % 2;
    } else if (ch == '1') {
      startNewGame();
    } else if (ch == '2' || ch == 'q' || ch == 'Q' || ch == 27) {
      quitRequested = true;
    } else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      if (selectedMenu == 0) {
        startNewGame();
      } else {
        quitRequested = true;
      }
    }
    return;
  }

  if (screenState == ScreenState::GameOver) {
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'r' ||
        ch == 'R') {
      startNewGame();
    } else if (ch == 'q' || ch == 'Q' || ch == 27) {
      quitRequested = true;
    } else if (ch == 'm' || ch == 'M') {
      screenState = ScreenState::MainMenu;
    }
    return;
  }

  switch (ch) {
  case 'w':
    movePlayer({0, -1}, false);
    break;
  case 'a':
    movePlayer({-1, 0}, false);
    break;
  case 's':
    movePlayer({0, 1}, false);
    break;
  case 'd':
    movePlayer({1, 0}, false);
    break;
  case 'W':
    movePlayer({0, -1}, true);
    break;
  case 'A':
    movePlayer({-1, 0}, true);
    break;
  case 'S':
    movePlayer({0, 1}, true);
    break;
  case 'D':
    movePlayer({1, 0}, true);
    break;
  case KEY_UP:
    movePlayer({0, -1}, false);
    break;
  case KEY_LEFT:
    movePlayer({-1, 0}, false);
    break;
  case KEY_DOWN:
    movePlayer({0, 1}, false);
    break;
  case KEY_RIGHT:
    movePlayer({1, 0}, false);
    break;
  case 'q':
  case 'Q':
  case 27:
    quitRequested = true;
    break;
  default:
    break;
  }
}

void Game::handleMouseClick(Point worldPoint) {
  if (screenState != ScreenState::Playing) {
    return;
  }
  shootAt(worldPoint);
}

void Game::startNewGame() {
  screenState = ScreenState::Playing;
  currentLevel = 1;
  player = Player{};
  achievements = Achievements{};
  achievements.killsByKind[MonsterKind::Goblin] = 0;
  achievements.killsByKind[MonsterKind::Skeleton] = 0;
  achievements.killsByKind[MonsterKind::Orc] = 0;
  achievements.killsByKind[MonsterKind::Troll] = 0;
  achievements.killsByKind[MonsterKind::Dragon] = 0;
  sprintCooldown = 0;
  playerInvincibleTicks = 0;
  statusMessage = "左键射击，找到出口进入下一关。";
  buildLevel();
}

void Game::update() {
  if (screenState != ScreenState::Playing) {
    return;
  }

  ++frame;
  if (sprintCooldown > 0) {
    --sprintCooldown;
  }
  if (playerInvincibleTicks > 0) {
    --playerInvincibleTicks;
  }
  if (messageTimer > 0) {
    --messageTimer;
  }

  updateBullets();
  updateMonsters();

  if (player.hp <= 0) {
    player.hp = 0;
    screenState = ScreenState::GameOver;
    setMessage("你倒在了地牢里。按 Enter 重新开始。", 240);
  }
}

void Game::buildLevel() {
  maze.generate(currentLevel);
  player.pos = maze.start();
  bullets.clear();
  monsters.clear();
  pickups.clear();
  levelTookDamage = false;
  playerInvincibleTicks = 0;

  spawnMonsters();
  spawnPickups();
}

void Game::spawnMonsters() {
  auto blocked = [this](Point p) { return occupiedByActor(p) || p == maze.exit(); };

  std::vector<MonsterKind> plan;
  const int goblins = 2 + currentLevel;
  const int skeletons = 1 + currentLevel / 2;
  const int orcs = 1 + currentLevel / 2;
  const int trolls = currentLevel >= 2 ? 1 + currentLevel / 3 : 0;
  const int dragons = currentLevel >= 3 ? 1 + currentLevel / 4 : 0;

  plan.insert(plan.end(), goblins, MonsterKind::Goblin);
  plan.insert(plan.end(), skeletons, MonsterKind::Skeleton);
  plan.insert(plan.end(), orcs, MonsterKind::Orc);
  plan.insert(plan.end(), trolls, MonsterKind::Troll);
  plan.insert(plan.end(), dragons, MonsterKind::Dragon);
  std::shuffle(plan.begin(), plan.end(), rng);

  for (MonsterKind kind : plan) {
    Monster monster = makeMonster(kind);
    monster.pos = maze.randomFloor(blocked);
    if (distanceManhattan(monster.pos, player.pos) < 8) {
      monster.pos = maze.randomFloor(blocked);
    }
    monsters.push_back(monster);
  }
}

void Game::spawnPickups() {
  auto blocked = [this](Point p) {
    if (occupiedByActor(p) || p == maze.exit()) {
      return true;
    }
    return std::any_of(pickups.begin(), pickups.end(),
                       [p](const Pickup &pickup) { return pickup.pos == p; });
  };

  const int healthCount = 2 + currentLevel / 2;
  const int ammoCount = 3 + currentLevel / 3;
  for (int i = 0; i < healthCount; ++i) {
    pickups.push_back({maze.randomFloor(blocked), PickupKind::Health});
  }
  for (int i = 0; i < ammoCount; ++i) {
    pickups.push_back({maze.randomFloor(blocked), PickupKind::Ammo});
  }
}

void Game::movePlayer(Point delta, bool sprint) {
  int steps = 1;
  if (sprint) {
    if (sprintCooldown == 0) {
      steps = 2;
      sprintCooldown = sprintCooldownMax;
    } else {
      setMessage("疾跑还在冷却。", 35);
    }
  }

  for (int i = 0; i < steps; ++i) {
    Point next = player.pos + delta;
    if (!maze.isWalkable(next)) {
      break;
    }

    if (auto index = monsterAt(next)) {
      damagePlayer(monsters[*index].damage, monsterName(monsters[*index].kind) +
                                           "挡住了你。");
      break;
    }

    player.pos = next;

    if (auto pickupIndex = pickupAt(player.pos)) {
      Pickup pickup = pickups[*pickupIndex];
      pickups.erase(pickups.begin() + static_cast<long>(*pickupIndex));
      if (pickup.kind == PickupKind::Health) {
        player.hp = std::min(player.maxHp, player.hp + 28);
        setMessage("拾取了生命补给，HP 恢复。");
      } else {
        player.ammo = std::min(player.maxAmmo, player.ammo + 12);
        setMessage("拾取了弹药补给。");
      }
    }

    if (player.pos == maze.exit()) {
      finishLevel();
      break;
    }
  }
}

void Game::shootAt(Point target) {
  if (target == player.pos) {
    return;
  }
  if (player.ammo <= 0) {
    setMessage("没有弹药了，去找 A 弹药点。", 70);
    return;
  }

  std::vector<Point> path = buildBulletPath(player.pos, target);
  if (path.empty()) {
    return;
  }

  --player.ammo;
  Bullet bullet;
  bullet.pos = player.pos;
  bullet.path = std::move(path);
  bullet.damage = 26 + currentLevel * 2;
  bullets.push_back(std::move(bullet));
}

void Game::updateBullets() {
  for (Bullet &bullet : bullets) {
    if (!bullet.alive) {
      continue;
    }
    if (bullet.nextIndex >= bullet.path.size()) {
      bullet.alive = false;
      continue;
    }

    bullet.pos = bullet.path[bullet.nextIndex++];
    if (maze.isWall(bullet.pos)) {
      bullet.alive = false;
      continue;
    }

    if (auto index = monsterAt(bullet.pos)) {
      Monster &monster = monsters[*index];
      monster.hp -= bullet.damage;
      bullet.alive = false;
      if (monster.hp <= 0) {
        monster.alive = false;
        achievements.killsByKind[monster.kind]++;
        setMessage("击杀 " + monsterName(monster.kind) + "。");
      } else {
        setMessage(monsterName(monster.kind) + " 受伤。", 35);
      }
    }
  }

  bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                               [](const Bullet &bullet) {
                                 return !bullet.alive;
                               }),
                bullets.end());
  monsters.erase(std::remove_if(monsters.begin(), monsters.end(),
                                [](const Monster &monster) {
                                  return !monster.alive;
                                }),
                 monsters.end());
}

void Game::updateMonsters() {
  for (Monster &monster : monsters) {
    if (!monster.alive) {
      continue;
    }

    if (distanceManhattan(monster.pos, player.pos) <= 1) {
      damagePlayer(monster.damage, monsterName(monster.kind) + "近身攻击。");
      continue;
    }

    if (monster.kind == MonsterKind::Dragon) {
      // 幼龙不移动，但会在直线或斜线视野中喷火。
      if (frame % (kFramesPerSecond * 2) == 0 &&
          distanceManhattan(monster.pos, player.pos) <= 12 &&
          sameLineOrDiagonal(monster.pos, player.pos) &&
          maze.hasLineOfSight(monster.pos, player.pos)) {
        damagePlayer(monster.damage, "幼龙的火焰命中你。");
      }
      continue;
    }

    if (++monster.moveTimer < monster.moveDelay) {
      continue;
    }
    monster.moveTimer = 0;

    Point step = chooseMonsterStep(monster);
    Point next = monster.pos + step;
    if (step.x == 0 && step.y == 0) {
      continue;
    }
    if (next == player.pos) {
      damagePlayer(monster.damage, monsterName(monster.kind) + "撞上了你。");
      continue;
    }
    if (maze.isWalkable(next) && !occupiedByActor(next)) {
      monster.pos = next;
    }
  }
}

void Game::finishLevel() {
  achievements.levelsCleared++;
  if (!levelTookDamage) {
    achievements.flawlessClears++;
  }

  ++currentLevel;
  player.hp = std::min(player.maxHp, player.hp + 18);
  player.ammo = std::min(player.maxAmmo, player.ammo + 8);
  sprintCooldown = 0;
  setMessage("进入第 " + std::to_string(currentLevel) + " 关。", 110);
  buildLevel();
}

void Game::damagePlayer(int amount, const std::string &reason) {
  if (amount <= 0 || player.hp <= 0) {
    return;
  }
  if (playerInvincibleTicks > 0) {
    return;
  }
  player.hp -= amount;
  playerInvincibleTicks = kPlayerInvincibleTicks;
  levelTookDamage = true;
  setMessage(reason + " -" + std::to_string(amount) + "HP", 65);
}

void Game::setMessage(const std::string &message, int frames) {
  statusMessage = message;
  messageTimer = frames;
}

std::optional<std::size_t> Game::monsterAt(Point p) const {
  for (std::size_t i = 0; i < monsters.size(); ++i) {
    if (monsters[i].alive && monsters[i].pos == p) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> Game::pickupAt(Point p) const {
  for (std::size_t i = 0; i < pickups.size(); ++i) {
    if (pickups[i].pos == p) {
      return i;
    }
  }
  return std::nullopt;
}

bool Game::occupiedByActor(Point p) const {
  if (player.pos == p) {
    return true;
  }
  return std::any_of(monsters.begin(), monsters.end(),
                     [p](const Monster &monster) {
                       return monster.alive && monster.pos == p;
                     });
}

Monster Game::makeMonster(MonsterKind kind) const {
  const int scale = std::max(0, currentLevel - 1);
  Monster monster;
  monster.kind = kind;
  switch (kind) {
  case MonsterKind::Goblin:
    monster.maxHp = 22 + scale * 3;
    monster.damage = 5 + scale;
    monster.followRange = 8;
    monster.moveDelay = 7;
    break;
  case MonsterKind::Skeleton:
    monster.maxHp = 28 + scale * 4;
    monster.damage = 7 + scale;
    monster.followRange = 10;
    monster.moveDelay = 6;
    break;
  case MonsterKind::Orc:
    monster.maxHp = 38 + scale * 5;
    monster.damage = 10 + scale * 2;
    monster.followRange = 13;
    monster.moveDelay = 9;
    break;
  case MonsterKind::Troll:
    monster.maxHp = 62 + scale * 8;
    monster.damage = 14 + scale * 2;
    monster.followRange = 9;
    monster.moveDelay = 13;
    break;
  case MonsterKind::Dragon:
    monster.maxHp = 70 + scale * 10;
    monster.damage = 13 + scale * 2;
    monster.followRange = 12;
    monster.moveDelay = 1000;
    break;
  }
  monster.hp = monster.maxHp;
  return monster;
}

Point Game::chooseMonsterStep(const Monster &monster) {
  if (distanceManhattan(monster.pos, player.pos) <= monster.followRange) {
    if (monster.kind == MonsterKind::Orc) {
      return nextStepByBfs(monster.pos, player.pos);
    }
    if (monster.kind == MonsterKind::Skeleton) {
      std::uniform_int_distribution<int> chance(0, 99);
      if (chance(rng) < 70) {
        Point diff = player.pos - monster.pos;
        return {signOf(diff.x), signOf(diff.y)};
      }
    } else {
      Point diff = player.pos - monster.pos;
      return {signOf(diff.x), signOf(diff.y)};
    }
  }

  static const std::array<Point, 8> dirs = {
      Point{1, 0},  Point{-1, 0}, Point{0, 1},  Point{0, -1},
      Point{1, 1},  Point{1, -1}, Point{-1, 1}, Point{-1, -1}};
  std::uniform_int_distribution<std::size_t> dist(0, dirs.size() - 1);
  return dirs[dist(rng)];
}

Point Game::nextStepByBfs(Point from, Point to) const {
  std::queue<Point> frontier;
  std::vector<std::vector<bool>> visited(
      maze.height(), std::vector<bool>(maze.width(), false));
  std::vector<std::vector<Point>> previous(
      maze.height(), std::vector<Point>(maze.width(), {-1, -1}));

  frontier.push(from);
  visited[from.y][from.x] = true;
  static const std::array<Point, 4> dirs = {
      Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}};

  while (!frontier.empty()) {
    Point current = frontier.front();
    frontier.pop();
    if (current == to) {
      break;
    }
    for (Point dir : dirs) {
      Point next = current + dir;
      if (!maze.inBounds(next) || visited[next.y][next.x]) {
        continue;
      }
      if (next != to && (!maze.isWalkable(next) || occupiedByActor(next))) {
        continue;
      }
      visited[next.y][next.x] = true;
      previous[next.y][next.x] = current;
      frontier.push(next);
    }
  }

  if (!visited[to.y][to.x]) {
    Point diff = to - from;
    return {signOf(diff.x), signOf(diff.y)};
  }

  Point step = to;
  while (previous[step.y][step.x] != from &&
         previous[step.y][step.x] != Point{-1, -1}) {
    step = previous[step.y][step.x];
  }
  return step - from;
}

std::vector<Point> Game::buildBulletPath(Point from, Point to) const {
  std::vector<Point> path;
  int x0 = from.x;
  int y0 = from.y;
  const int x1 = to.x;
  const int y1 = to.y;
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (true) {
    if (!(x0 == from.x && y0 == from.y)) {
      Point p{x0, y0};
      if (!maze.inBounds(p)) {
        break;
      }
      path.push_back(p);
      if (maze.isWall(p) || p == to) {
        break;
      }
    }

    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int doubled = 2 * error;
    if (doubled >= dy) {
      error += dy;
      x0 += sx;
    }
    if (doubled <= dx) {
      error += dx;
      y0 += sy;
    }
  }
  return path;
}
