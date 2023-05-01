#include <cstdarg>
#include <cstdio>
#include <sstream>

#include "term.hpp"

// super useful references:
// https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences
// https://www2.math.upenn.edu/~kazdan/210/computer/ansi.html

void term::clear_screen() {
  printf("\033[2J");
}

void term::clear_curr_line() {
  printf("\33[2K\r");
}

void term::clear_to_end_of_line() {
  printf("\033[K");
}

void term::cursor::hide() {
  printf("\33[?25l");
}

void term::cursor::show() {
  printf("\33[?25h");
}

void term::cursor::disable_blinking() {
  printf("\33[?12l");
}

void term::cursor::move_to(size_t const x, size_t const y) {
  // row;col
  // ^^^ ^^^
  // start from 1 rather than 0 so we must add 1 to `x` and `y` accordingly
  printf("\33[%zu;%zuH", y + 1, x + 1);
}

void term::cursor::move_up(size_t const n) {
  printf("\33[%zuA", n);
}

void term::cursor::move_down(size_t const n) {
  printf("\33[%zuB", n);
}

void term::cursor::move_right(size_t const n) {
  printf("\33[%zuC", n);
}

void term::cursor::move_left(size_t const n) {
  printf("\33[%zuD", n);
}

void term::cursor::move_to_top_left() {
  printf("\33[;H");
}

void term::cursor::save_pos() {
  printf("\33[s");
}

void term::cursor::restore_pos() {
  printf("\33[u");
}

static
int ansi_color_code_fore(term::color::value_type const color) {
  term::color::value_type const fg_component = color & 0b00000000000000001111111111111111;

  int ansi_color_code;

  switch (fg_component) {
    using namespace term::color;

    case fore::DEFAULT: ansi_color_code = 0; break;
    case fore::RED: ansi_color_code = 31; break;
    case fore::GREEN: ansi_color_code = 32; break;
    case fore::YELLOW: ansi_color_code = 33; break;
    case fore::BLUE: ansi_color_code = 34; break;
    case fore::MAGENTA: ansi_color_code = 35; break;
    case fore::CYAN: ansi_color_code = 36; break;

    case fore::GRAY: ansi_color_code = 90; break;
    case fore::GREY: ansi_color_code = 90; break;
    case fore::LIGHT_RED: ansi_color_code = 91; break;
    case fore::LIGHT_GREEN: ansi_color_code = 92; break;
    case fore::LIGHT_YELLOW: ansi_color_code = 93; break;
    case fore::LIGHT_BLUE: ansi_color_code = 94; break;
    case fore::LIGHT_MAGENTA: ansi_color_code = 95; break;
    case fore::LIGHT_CYAN: ansi_color_code = 96; break;
    case fore::WHITE: ansi_color_code = 97; break;

    default: throw std::runtime_error(
      "ansi_color_code_fore: bad term::color::fore `color`"
    );
  }

  return ansi_color_code;
}

static
int ansi_color_code_back(term::color::value_type const color) {
  term::color::value_type const bg_component = color & 0b11111111100000000000000000000000;

  int ansi_color_code;

  switch (bg_component) {
    using namespace term::color;

    case back::BLACK: ansi_color_code = 40; break;
    case back::RED: ansi_color_code = 41; break;
    case back::GREEN: ansi_color_code = 42; break;
    case back::YELLOW: ansi_color_code = 43; break;
    case back::BLUE: ansi_color_code = 44; break;
    case back::MAGENTA: ansi_color_code = 45; break;
    case back::CYAN: ansi_color_code = 46; break;
    case back::WHITE: ansi_color_code = 47; break;
    case back::DEFAULT: ansi_color_code = 49; break;

    default: throw std::runtime_error(
      "ansi_color_code_back: bad term::color::back `color`"
    );
  }

  return ansi_color_code;
}

void term::color::set(term::color::value_type const color) {
  if (color & 0b00000000000000001111111111111111) {
    int const fg = ansi_color_code_fore(color);
    std::printf("\033[%dm", fg);
  }
  if (color & 0b11111111100000000000000000000000) {
    int const bg = ansi_color_code_back(color);
    std::printf("\033[%dm", bg);
  }
}

void term::color::printf(term::color::value_type const color, char const *fmt, ...) {
  color::set(color);

  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);

  color::set(fore::DEFAULT | back::DEFAULT);
}
