#ifndef ANT_SIMULATOR_LITE_UI_PRINT_HPP
#define ANT_SIMULATOR_LITE_UI_PRINT_HPP

#include <stddef.h>

enum class TextColor {
  Default = 0,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  Gray = 90
};

void print_banner();
void print_horizontal_rule();
void printfc(TextColor color, char const * format, ...);
void stdout_cursor_hide();
void stdout_cursor_show();
void stdout_cursor_move_up(size_t count);
void stdout_clear_current_line();

#endif // ANT_SIMULATOR_LITE_UI_PRINT_HPP
