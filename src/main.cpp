#include "Game.h"
#include "Renderer.h"

#include <chrono>
#include <clocale>
#include <ncurses.h>
#include <thread>

class CursesSession {
public:
  CursesSession() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    mousemask(BUTTON1_PRESSED | BUTTON1_CLICKED, nullptr);
    mouseinterval(0);
  }

  ~CursesSession() { endwin(); }
};

int main() {
  CursesSession terminal;
  Game game;
  Renderer renderer;

  using clock = std::chrono::steady_clock;
  constexpr auto frameTime = std::chrono::milliseconds(33);

  while (!game.wantsQuit()) {
    renderer.draw(game);

    int ch = 0;
    while ((ch = getch()) != ERR) {
      if (ch == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK &&
            (event.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED))) {
          if (auto world = renderer.screenToWorld(event.x, event.y)) {
            game.handleMouseClick(*world);
          }
        }
      } else {
        game.handleKey(ch);
      }
    }

    const auto begin = clock::now();
    game.update();
    const auto elapsed = clock::now() - begin;
    if (elapsed < frameTime) {
      std::this_thread::sleep_for(frameTime - elapsed);
    }
  }

  return 0;
}
