#include "Maze.h"

#include <algorithm>
#include <cmath>
#include <ctime>

Maze::Maze(int width, int height)
    : widthValue(width), heightValue(height),
      rng(static_cast<unsigned int>(std::time(nullptr))) {}

void Maze::generate(int level) {
  // 每一关稍微改变随机种子，避免短时间连续开局时地图完全一样。
  rng.seed(static_cast<unsigned int>(std::time(nullptr)) +
           static_cast<unsigned int>(level * 7919));

  grid.assign(heightValue, std::string(widthValue, '#'));
  rooms.clear();

  BSPNode root;
  root.x = 1;
  root.y = 1;
  root.width = widthValue - 2;
  root.height = heightValue - 2;

  split(root, 0);
  createRooms(root);
  connectRooms(root);

  if (rooms.empty()) {
    // 极小终端或异常尺寸下的兜底地图，保证游戏仍能运行。
    Room fallback{2, 2, std::max(6, widthValue - 4),
                  std::max(6, heightValue - 4)};
    rooms.push_back(fallback);
    carveRoom(fallback);
  }

  auto startRoom = std::min_element(
      rooms.begin(), rooms.end(), [](const Room &a, const Room &b) {
        return a.x + a.y < b.x + b.y;
      });
  auto exitRoom = std::max_element(
      rooms.begin(), rooms.end(), [](const Room &a, const Room &b) {
        return a.x + a.y < b.x + b.y;
      });

  startPos = startRoom->center();
  exitPos = exitRoom->center();
  grid[startPos.y][startPos.x] = ' ';
  grid[exitPos.y][exitPos.x] = ' ';
}

bool Maze::inBounds(Point p) const {
  return p.x >= 0 && p.y >= 0 && p.x < widthValue && p.y < heightValue;
}

bool Maze::isWall(Point p) const {
  return !inBounds(p) || grid[p.y][p.x] == '#';
}

bool Maze::isWalkable(Point p) const { return inBounds(p) && !isWall(p); }

Point Maze::randomFloor(const std::function<bool(Point)> &blocked) {
  std::uniform_int_distribution<int> xDist(1, widthValue - 2);
  std::uniform_int_distribution<int> yDist(1, heightValue - 2);

  for (int attempt = 0; attempt < 3000; ++attempt) {
    Point p{xDist(rng), yDist(rng)};
    if (isWalkable(p) && !blocked(p)) {
      return p;
    }
  }

  // 随机失败时顺序扫描，尽量不要因为地图太挤而崩溃。
  for (int y = 1; y < heightValue - 1; ++y) {
    for (int x = 1; x < widthValue - 1; ++x) {
      Point p{x, y};
      if (isWalkable(p) && !blocked(p)) {
        return p;
      }
    }
  }
  return startPos;
}

bool Maze::hasLineOfSight(Point from, Point to) const {
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
    Point current{x0, y0};
    if (current != from && current != to && isWall(current)) {
      return false;
    }
    if (x0 == x1 && y0 == y1) {
      return true;
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
}

void Maze::split(BSPNode &node, int depth) {
  const int minPartition = 14;
  const int maxDepth = 6;

  if (depth >= maxDepth ||
      (node.width < minPartition * 2 && node.height < minPartition * 2)) {
    return;
  }

  bool splitHorizontal = false;
  if (node.width > static_cast<int>(node.height * 1.25)) {
    splitHorizontal = false;
  } else if (node.height > static_cast<int>(node.width * 1.25)) {
    splitHorizontal = true;
  } else {
    std::uniform_int_distribution<int> coin(0, 1);
    splitHorizontal = coin(rng) == 0;
  }

  int length = splitHorizontal ? node.height : node.width;
  if (length < minPartition * 2) {
    return;
  }

  std::uniform_int_distribution<int> splitDist(minPartition,
                                               length - minPartition);
  const int cut = splitDist(rng);

  node.left = std::make_unique<BSPNode>();
  node.right = std::make_unique<BSPNode>();

  if (splitHorizontal) {
    node.left->x = node.x;
    node.left->y = node.y;
    node.left->width = node.width;
    node.left->height = cut;

    node.right->x = node.x;
    node.right->y = node.y + cut;
    node.right->width = node.width;
    node.right->height = node.height - cut;
  } else {
    node.left->x = node.x;
    node.left->y = node.y;
    node.left->width = cut;
    node.left->height = node.height;

    node.right->x = node.x + cut;
    node.right->y = node.y;
    node.right->width = node.width - cut;
    node.right->height = node.height;
  }

  split(*node.left, depth + 1);
  split(*node.right, depth + 1);
}

void Maze::createRooms(BSPNode &node) {
  if (node.left || node.right) {
    if (node.left) {
      createRooms(*node.left);
    }
    if (node.right) {
      createRooms(*node.right);
    }
    return;
  }

  const int padding = 2;
  const int minRoomWidth = 6;
  const int minRoomHeight = 5;
  const int maxRoomWidth = std::max(minRoomWidth, node.width - padding * 2);
  const int maxRoomHeight = std::max(minRoomHeight, node.height - padding * 2);

  if (node.width < minRoomWidth + padding * 2 ||
      node.height < minRoomHeight + padding * 2) {
    return;
  }

  std::uniform_int_distribution<int> widthDist(
      minRoomWidth, std::min(maxRoomWidth, 18));
  std::uniform_int_distribution<int> heightDist(
      minRoomHeight, std::min(maxRoomHeight, 12));

  Room room;
  room.width = widthDist(rng);
  room.height = heightDist(rng);

  std::uniform_int_distribution<int> xDist(
      node.x + padding, node.x + node.width - room.width - padding);
  std::uniform_int_distribution<int> yDist(
      node.y + padding, node.y + node.height - room.height - padding);

  room.x = xDist(rng);
  room.y = yDist(rng);
  node.room = room;
  node.hasRoom = true;
  rooms.push_back(room);
  carveRoom(room);
}

Point Maze::connectRooms(BSPNode &node) {
  if (node.hasRoom) {
    return node.room.center();
  }

  Point leftCenter{-1, -1};
  Point rightCenter{-1, -1};
  if (node.left) {
    leftCenter = connectRooms(*node.left);
  }
  if (node.right) {
    rightCenter = connectRooms(*node.right);
  }

  if (leftCenter.x >= 0 && rightCenter.x >= 0) {
    // L 形走廊连接两个子区域，类似参考工程的 BSP 房间连接方式。
    std::uniform_int_distribution<int> coin(0, 1);
    if (coin(rng) == 0) {
      carveHorizontal(leftCenter.x, rightCenter.x, leftCenter.y);
      carveVertical(leftCenter.y, rightCenter.y, rightCenter.x);
    } else {
      carveVertical(leftCenter.y, rightCenter.y, leftCenter.x);
      carveHorizontal(leftCenter.x, rightCenter.x, rightCenter.y);
    }
    return {(leftCenter.x + rightCenter.x) / 2,
            (leftCenter.y + rightCenter.y) / 2};
  }

  return leftCenter.x >= 0 ? leftCenter : rightCenter;
}

void Maze::carveRoom(const Room &room) {
  for (int y = room.y; y < room.y + room.height; ++y) {
    for (int x = room.x; x < room.x + room.width; ++x) {
      if (inBounds({x, y})) {
        grid[y][x] = ' ';
      }
    }
  }
}

void Maze::carveHorizontal(int x1, int x2, int y) {
  const int minX = std::min(x1, x2);
  const int maxX = std::max(x1, x2);
  for (int x = minX; x <= maxX; ++x) {
    if (inBounds({x, y})) {
      grid[y][x] = ' ';
    }
  }
}

void Maze::carveVertical(int y1, int y2, int x) {
  const int minY = std::min(y1, y2);
  const int maxY = std::max(y1, y2);
  for (int y = minY; y <= maxY; ++y) {
    if (inBounds({x, y})) {
      grid[y][x] = ' ';
    }
  }
}
