#ifndef ASCII_DUNGEON_RENDERER_H
#define ASCII_DUNGEON_RENDERER_H

#include "Game.h"

#include <optional>

class Renderer {
public:
  Renderer();

  void draw(const Game &game);
  std::optional<Point> screenToWorld(int screenX, int screenY) const;

private:
  struct Viewport {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    Point origin{0, 0};
  };

  Viewport viewport;

  void initColors();
  void drawMenu(const Game &game);
  void drawPlaying(const Game &game);
  void drawGameOver(const Game &game);
  void drawGlyph(Point world, char glyph, int colorPair, int attrs = 0);
  void drawBar(int row, int col, int width, int value, int maxValue,
               int colorPair);
};

#endif
