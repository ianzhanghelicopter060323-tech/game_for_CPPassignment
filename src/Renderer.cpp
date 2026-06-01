#include "Renderer.h"

#include <algorithm>
#include <array>
#include <ncurses.h>
#include <string>

namespace {
enum ColorPair {
  PairWall = 1,
  PairFloor,
  PairPlayer,
  PairMonster,
  PairExit,
  PairHealth,
  PairAmmo,
  PairBullet,
  PairUi,
  PairWarn,
  PairTitle
};

std::string cooldownText(int ticks) {
  if (ticks <= 0) {
    return "就绪";
  }
  const int tenths = (ticks * 10 + 29) / 30;
  return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) +
         "s";
}

std::string killsCompact(const Achievements &ach) {
  auto countOf = [&ach](MonsterKind kind) {
    auto found = ach.killsByKind.find(kind);
    return found == ach.killsByKind.end() ? 0 : found->second;
  };
  return "击杀 g:" + std::to_string(countOf(MonsterKind::Goblin)) +
         " s:" + std::to_string(countOf(MonsterKind::Skeleton)) +
         " o:" + std::to_string(countOf(MonsterKind::Orc)) +
         " T:" + std::to_string(countOf(MonsterKind::Troll)) +
         " D:" + std::to_string(countOf(MonsterKind::Dragon));
}

void printClipped(int row, int col, int width, const std::string &text,
                  int attrs = 0) {
  if (row < 0 || col < 0 || width <= 0) {
    return;
  }
  std::string clipped = text.substr(0, static_cast<std::size_t>(width));
  attron(attrs);
  mvprintw(row, col, "%s", clipped.c_str());
  attroff(attrs);
}
} // namespace

Renderer::Renderer() { initColors(); }

void Renderer::draw(const Game &game) {
  erase();
  switch (game.screen()) {
  case ScreenState::MainMenu:
    drawMenu(game);
    break;
  case ScreenState::Playing:
    drawPlaying(game);
    break;
  case ScreenState::GameOver:
    drawGameOver(game);
    break;
  }
  wnoutrefresh(stdscr);
  doupdate();
}

std::optional<Point> Renderer::screenToWorld(int screenX, int screenY) const {
  if (screenX < viewport.left || screenY < viewport.top ||
      screenX >= viewport.left + viewport.width ||
      screenY >= viewport.top + viewport.height) {
    return std::nullopt;
  }
  return Point{viewport.origin.x + screenX - viewport.left,
               viewport.origin.y + screenY - viewport.top};
}

void Renderer::initColors() {
  if (!has_colors()) {
    return;
  }
  start_color();
  use_default_colors();
  init_pair(PairWall, COLOR_WHITE, -1);
  init_pair(PairFloor, COLOR_BLUE, -1);
  init_pair(PairPlayer, COLOR_CYAN, -1);
  init_pair(PairMonster, COLOR_RED, -1);
  init_pair(PairExit, COLOR_GREEN, -1);
  init_pair(PairHealth, COLOR_GREEN, -1);
  init_pair(PairAmmo, COLOR_YELLOW, -1);
  init_pair(PairBullet, COLOR_YELLOW, -1);
  init_pair(PairUi, COLOR_WHITE, -1);
  init_pair(PairWarn, COLOR_RED, -1);
  init_pair(PairTitle, COLOR_MAGENTA, -1);
}

void Renderer::drawMenu(const Game &game) {
  viewport = Viewport{};
  int rows = 0;
  int cols = 0;
  getmaxyx(stdscr, rows, cols);

  const int titleRow = std::max(2, rows / 4);
  const std::string title = "ASCII 迷宫枪战";
  const std::string subTitle = "WASD 移动  鼠标左键射击  Shift+WASD 疾跑";
  printClipped(titleRow, std::max(0, cols / 2 - 10), cols, title,
               A_BOLD | COLOR_PAIR(PairTitle));
  printClipped(titleRow + 2, std::max(0, cols / 2 - 24), cols, subTitle,
               COLOR_PAIR(PairUi));

  const std::array<std::string, 2> options = {"1. 新游戏", "2. 退出游戏"};
  for (int i = 0; i < 2; ++i) {
    int attrs = COLOR_PAIR(PairUi);
    if (game.selectedMenuIndex() == i) {
      attrs |= A_REVERSE | A_BOLD;
    }
    printClipped(titleRow + 5 + i * 2, std::max(0, cols / 2 - 8), 24,
                 options[static_cast<std::size_t>(i)], attrs);
  }

  printClipped(rows - 2, 2, cols - 4,
               "按数字或 Enter 选择，W/S 切换。", COLOR_PAIR(PairUi));
}

void Renderer::drawPlaying(const Game &game) {
  int rows = 0;
  int cols = 0;
  getmaxyx(stdscr, rows, cols);

  const Maze &maze = game.getMaze();
  const Player &player = game.getPlayer();
  const bool hasSidebar = cols >= 92;
  const int sidebarWidth = hasSidebar ? 30 : 0;
  const int bottomHud = hasSidebar ? 1 : 5;

  viewport.left = 1;
  viewport.top = 1;
  viewport.width = std::max(1, std::min(maze.width(), cols - sidebarWidth - 3));
  viewport.height = std::max(1, std::min(maze.height(), rows - bottomHud - 1));
  viewport.origin.x =
      std::clamp(player.pos.x - viewport.width / 2, 0,
                 std::max(0, maze.width() - viewport.width));
  viewport.origin.y =
      std::clamp(player.pos.y - viewport.height / 2, 0,
                 std::max(0, maze.height() - viewport.height));

  for (int y = 0; y < viewport.height; ++y) {
    for (int x = 0; x < viewport.width; ++x) {
      Point world{viewport.origin.x + x, viewport.origin.y + y};
      char tile = maze.tiles()[world.y][world.x];
      if (tile == '#') {
        mvaddch(viewport.top + y, viewport.left + x,
                '#' | COLOR_PAIR(PairWall) | A_BOLD);
      } else {
        mvaddch(viewport.top + y, viewport.left + x,
                '.' | COLOR_PAIR(PairFloor) | A_DIM);
      }
    }
  }

  drawGlyph(maze.exit(), '>', PairExit, A_BOLD);
  for (const Pickup &pickup : game.getPickups()) {
    if (pickup.kind == PickupKind::Health) {
      drawGlyph(pickup.pos, '+', PairHealth, A_BOLD);
    } else {
      drawGlyph(pickup.pos, 'A', PairAmmo, A_BOLD);
    }
  }
  for (const Bullet &bullet : game.getBullets()) {
    drawGlyph(bullet.pos, '*', PairBullet, A_BOLD);
  }
  for (const Monster &monster : game.getMonsters()) {
    drawGlyph(monster.pos, monsterGlyph(monster.kind), PairMonster, A_BOLD);
  }
  const int invincibleTicks = game.playerInvincibleTicksRemaining();
  const bool playerFlash = invincibleTicks > 0 && (invincibleTicks / 4) % 2 == 0;
  drawGlyph(player.pos, '@', playerFlash ? PairUi : PairPlayer,
            A_BOLD | (playerFlash ? A_REVERSE : 0));

  std::string topLine = "关卡 " + std::to_string(game.level()) +
                        "  HP " + std::to_string(player.hp) + "/" +
                        std::to_string(player.maxHp) + "  弹药 " +
                        std::to_string(player.ammo) + "/" +
                        std::to_string(player.maxAmmo) + "  疾跑 " +
                        cooldownText(game.sprintCooldownTicks());
  printClipped(0, 1, cols - 2, topLine, COLOR_PAIR(PairUi) | A_BOLD);

  if (hasSidebar) {
    const int x = viewport.left + viewport.width + 2;
    int row = 1;
    printClipped(row++, x, sidebarWidth, "状态", A_BOLD | COLOR_PAIR(PairTitle));
    drawBar(row++, x, 20, player.hp, player.maxHp, PairHealth);
    printClipped(row++, x, sidebarWidth,
                 "弹药: " + std::to_string(player.ammo) + "/" +
                     std::to_string(player.maxAmmo),
                 COLOR_PAIR(PairAmmo));
    printClipped(row++, x, sidebarWidth,
                 "疾跑CD: " + cooldownText(game.sprintCooldownTicks()),
                 COLOR_PAIR(PairUi));
    row++;
    printClipped(row++, x, sidebarWidth, "成就",
                 A_BOLD | COLOR_PAIR(PairTitle));
    const Achievements &ach = game.getAchievements();
    printClipped(row++, x, sidebarWidth,
                 "通关: " + std::to_string(ach.levelsCleared),
                 COLOR_PAIR(PairUi));
    printClipped(row++, x, sidebarWidth,
                 "无伤: " + std::to_string(ach.flawlessClears),
                 COLOR_PAIR(PairUi));
    for (const auto &entry : ach.killsByKind) {
      printClipped(row++, x, sidebarWidth,
                   monsterName(entry.first) + ": " +
                       std::to_string(entry.second),
                   COLOR_PAIR(PairUi));
    }
    row++;
    printClipped(row++, x, sidebarWidth, "操作",
                 A_BOLD | COLOR_PAIR(PairTitle));
    printClipped(row++, x, sidebarWidth, "WASD/方向键 移动", COLOR_PAIR(PairUi));
    printClipped(row++, x, sidebarWidth, "Shift+WASD 疾跑", COLOR_PAIR(PairUi));
    printClipped(row++, x, sidebarWidth, "鼠标左键 射击", COLOR_PAIR(PairUi));
    printClipped(row++, x, sidebarWidth, "q 退出", COLOR_PAIR(PairUi));
  } else {
    int row = viewport.top + viewport.height + 1;
    drawBar(row++, 1, std::min(22, cols - 2), player.hp, player.maxHp,
            PairHealth);
    printClipped(row++, 1, cols - 2,
                 "弹药 " + std::to_string(player.ammo) + "/" +
                     std::to_string(player.maxAmmo) + "  通关 " +
                     std::to_string(game.getAchievements().levelsCleared) +
                     "  无伤 " +
                     std::to_string(game.getAchievements().flawlessClears),
                 COLOR_PAIR(PairUi));
    printClipped(row++, 1, cols - 2, killsCompact(game.getAchievements()),
                 COLOR_PAIR(PairUi));
  }

  printClipped(rows - 1, 1, cols - 2, game.message(), COLOR_PAIR(PairWarn));
}

void Renderer::drawGameOver(const Game &game) {
  viewport = Viewport{};
  int rows = 0;
  int cols = 0;
  getmaxyx(stdscr, rows, cols);
  int row = std::max(2, rows / 4);
  printClipped(row++, std::max(0, cols / 2 - 6), cols, "游戏结束",
               A_BOLD | COLOR_PAIR(PairWarn));
  row++;
  const Achievements &ach = game.getAchievements();
  printClipped(row++, std::max(0, cols / 2 - 12), cols,
               "通过关卡: " + std::to_string(ach.levelsCleared),
               COLOR_PAIR(PairUi));
  printClipped(row++, std::max(0, cols / 2 - 12), cols,
               "无伤通关: " + std::to_string(ach.flawlessClears),
               COLOR_PAIR(PairUi));
  for (const auto &entry : ach.killsByKind) {
    printClipped(row++, std::max(0, cols / 2 - 12), cols,
                 monsterName(entry.first) + ": " +
                     std::to_string(entry.second),
                 COLOR_PAIR(PairUi));
  }
  row++;
  printClipped(row, std::max(0, cols / 2 - 18), cols,
               "Enter/R 重新开始，M 返回菜单，Q 退出。", COLOR_PAIR(PairTitle));
}

void Renderer::drawGlyph(Point world, char glyph, int colorPair, int attrs) {
  const int screenX = viewport.left + world.x - viewport.origin.x;
  const int screenY = viewport.top + world.y - viewport.origin.y;
  if (screenX < viewport.left || screenY < viewport.top ||
      screenX >= viewport.left + viewport.width ||
      screenY >= viewport.top + viewport.height) {
    return;
  }
  mvaddch(screenY, screenX, glyph | COLOR_PAIR(colorPair) | attrs);
}

void Renderer::drawBar(int row, int col, int width, int value, int maxValue,
                       int colorPair) {
  if (width <= 2 || maxValue <= 0) {
    return;
  }
  int filled = std::clamp(value * width / maxValue, 0, width);
  mvaddch(row, col, '[' | COLOR_PAIR(PairUi));
  for (int i = 0; i < width; ++i) {
    int attrs = i < filled ? (COLOR_PAIR(colorPair) | A_BOLD)
                           : (COLOR_PAIR(PairUi) | A_DIM);
    mvaddch(row, col + 1 + i, (i < filled ? '=' : '-') | attrs);
  }
  mvaddch(row, col + width + 1, ']' | COLOR_PAIR(PairUi));
}
