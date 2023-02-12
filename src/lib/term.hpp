#ifndef NLUKA_TERM_HPP
#define NLUKA_TERM_HPP

// Module for doing fancy things in the terminal via ANSI escape sequences.
// Make sure your terminal supports ANSI escape sequences when using this module!
namespace term {

void clear_screen();
void clear_curr_line();
void clear_to_end_of_line();

size_t width_in_cols();
size_t height_in_lines();

#ifdef _WIN32

struct Dimensions {
  size_t m_width;
  size_t m_height;
};

Dimensions dimensions();
void remove_scrollbar();

#endif // _WIN32

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

  #ifdef _WIN32

  struct Position {
    // starts from 0
    size_t m_x, m_y;
  };

  Position get_pos();
  void set_size(size_t percent);

  #endif // _WIN32

} // namespace cursor

namespace color {

  // Foreground.
  namespace fore {

    constexpr int
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

    constexpr int
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

  // Wrapper for `printf` allowing for colored printing.
  void printf(int color, char const *fmt, ...);

} // namespace color

} // namespace term

#endif // NLUKA_TERM_HPP
