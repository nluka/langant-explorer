#ifndef NLUKA_TERM_HPP
#define NLUKA_TERM_HPP

// Functions for doing fancy terminal stuff via ANSI escape sequences.
namespace term {

void clear_screen();
void clear_curr_line();
void clear_to_end_of_line();

namespace cursor {

  void hide();
  void show();
  void disable_blinking();

  void move_to(size_t col, size_t row);
  void move_to_top_left();
  void move_up(size_t lines);
  void move_down(size_t lines);
  void move_right(size_t cols);
  void move_left(size_t cols);
  void save_pos();
  void restore_pos();

} // namespace cursor

namespace color {

  // Foreground.
  namespace fore {
    int constexpr
      DEFAULT       = 0b00000000000000000000000000000000,
      RED           = 0b00000000000000000000000000000001,
      GREEN         = 0b00000000000000000000000000000010,
      YELLOW        = 0b00000000000000000000000000000100,
      BLUE          = 0b00000000000000000000000000001000,
      MAGENTA       = 0b00000000000000000000000000010000,
      CYAN          = 0b00000000000000000000000000100000,
      GRAY          = 0b00000000000000000000000001000000,
      GREY          = 0b00000000000000000000000010000000,
      LIGHT_RED     = 0b00000000000000000000000100000000,
      LIGHT_GREEN   = 0b00000000000000000000001000000000,
      LIGHT_YELLOW  = 0b00000000000000000000010000000000,
      LIGHT_BLUE    = 0b00000000000000000000100000000000,
      LIGHT_MAGENTA = 0b00000000000000000001000000000000,
      LIGHT_CYAN    = 0b00000000000000000010000000000000,
      WHITE         = 0b00000000000000000100000000000000;
  } // namespace fore

  // Background.
  namespace back {
    int constexpr
      BLACK   = 0b10000000000000000000000000000000,
      RED     = 0b01000000000000000000000000000000,
      GREEN   = 0b00100000000000000000000000000000,
      YELLOW  = 0b00010000000000000000000000000000,
      BLUE    = 0b00001000000000000000000000000000,
      MAGENTA = 0b00000100000000000000000000000000,
      CYAN    = 0b00000010000000000000000000000000,
      WHITE   = 0b00000001000000000000000000000000;
  } // namespace back

  // Sets stdout foreground and background color.
  void set(int color);

  // Wrapper for `printf` enabling colored printing.
  void printf(int color, char const *fmt, ...);

} // namespace color

} // namespace term

#endif // NLUKA_TERM_HPP
