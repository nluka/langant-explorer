#include <stdio.h>
#include <stdarg.h>
#include "../includes/term.hpp"

using term::ColorText;

static ColorText g_defaultColorText = ColorText::DEFAULT;

void term::set_color_text(ColorText const color) {
  printf("\033[%dm", static_cast<int>(color));
}

void term::cursor_hide() {
  printf("\33[?25l");
}
void term::cursor_show() {
  printf("\33[?25h");
}

void term::cursor_move_up(size_t const lines) {
  printf("\33[%zuA", lines);
}

void term::clear_curr_line() {
  printf("\33[2K\r");
}

void term::printf_colored(
  ColorText const color,
  char const *const format,
  ...
) {
  va_list args;
  va_start(args, format);
  set_color_text(color);
  vprintf(format, args);
  set_color_text(g_defaultColorText);
  va_end(args);
}

void term::set_color_text_default(ColorText const color) {
  g_defaultColorText = color;
}
