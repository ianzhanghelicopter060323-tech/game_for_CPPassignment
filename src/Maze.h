#ifndef ASCII_DUNGEON_MAZE_H
#define ASCII_DUNGEON_MAZE_H

#include "Types.h"

#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

struct Room {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  Point center() const { return {x + width / 2, y + height / 2}; }
};

// 使用 BSP(Binary Space Partition) 生成房间和走廊。
// 这个思路参考了 Asciiquest 工程中的 maze_generator。
class Maze {
public:
  Maze(int width = 96, int height = 32);

  void generate(int level);

  int width() const { return widthValue; }
  int height() const { return heightValue; }
  Point start() const { return startPos; }
  Point exit() const { return exitPos; }
  const std::vector<std::string> &tiles() const { return grid; }

  bool inBounds(Point p) const;
  bool isWall(Point p) const;
  bool isWalkable(Point p) const;
  Point randomFloor(const std::function<bool(Point)> &blocked);
  bool hasLineOfSight(Point from, Point to) const;

private:
  struct BSPNode {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::unique_ptr<BSPNode> left;
    std::unique_ptr<BSPNode> right;
    Room room;
    bool hasRoom = false;
  };

  int widthValue;
  int heightValue;
  Point startPos;
  Point exitPos;
  std::vector<std::string> grid;
  std::vector<Room> rooms;
  std::mt19937 rng;

  void split(BSPNode &node, int depth);
  void createRooms(BSPNode &node);
  Point connectRooms(BSPNode &node);
  bool hasPath(Point from, Point to) const;
  void ensureStartExitConnected();
  void carveRoom(const Room &room);
  void carveHorizontal(int x1, int x2, int y);
  void carveVertical(int y1, int y2, int x);
};

#endif
