#include <stdio.h>
#include <stdarg.h>
#include "print.hpp"
#include "../CommandVars.hpp"

void stdout_set_text_color(TextColor const color) {
  printf("\033[%dm", static_cast<int>(color));
}

void print_banner() {
  stdout_set_text_color(TextColor::Cyan);
  printf("\n");
  printf("   \\_|           \n");
  printf("   ( @)  _  __    \n");
  printf("    `(#)(#)(##)   \n");
  printf("     / |^`\\^`\\  \n");
  stdout_set_text_color(TextColor::Default);
  print_horizontal_rule();
  printf("ant simulator (lite)\n");
  print_horizontal_rule();
}

void printfc(TextColor const color, char const * const format, ...) {
  va_list args;
  va_start(args, format);
  stdout_set_text_color(color);
  vprintf(format, args);
  stdout_set_text_color(TextColor::Default);
  va_end(args);
}

void print_horizontal_rule() {
  printfc(TextColor::Gray, "--------------------\n");
}

void stdout_cursor_hide() {
  printf("\33[?25l");
}

void stdout_cursor_show() {
  printf("\33[?25h");
}

void stdout_cursor_move_up(size_t const count) {
  printf("\33[%zuA", count);
}

void stdout_clear_current_line() {
  printf("\33[2K\r");
}
