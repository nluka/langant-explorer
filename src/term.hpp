/*
  Modified version of https://github.com/nluka/cpp-term.
*/

#ifndef NLUKA_TERM_HPP
#define NLUKA_TERM_HPP

#include "primitives.hpp"

/// Functions for doing fancy terminal stuff via ANSI escape sequences.
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

    typedef u32 value_type;

    /// Foreground color constants.
    namespace fore {
      color::value_type constexpr
        DEFAULT       = 0u,
        RED           = 1u,
        GREEN         = (1u << 1),
        YELLOW        = (1u << 2),
        BLUE          = (1u << 3),
        MAGENTA       = (1u << 4),
        CYAN          = (1u << 5),
        GRAY          = (1u << 6),
        GREY          = (1u << 7),
        LIGHT_RED     = (1u << 8),
        LIGHT_GREEN   = (1u << 9),
        LIGHT_YELLOW  = (1u << 10),
        LIGHT_BLUE    = (1u << 11),
        LIGHT_MAGENTA = (1u << 12),
        LIGHT_CYAN    = (1u << 13),
        WHITE         = (1u << 14),
        num_bits      = 16;
    } // namespace fore

    /// Background color constants.
    namespace back {
      color::value_type constexpr
        BLACK    = (1u << 31),
        RED      = (1u << 30),
        GREEN    = (1u << 29),
        YELLOW   = (1u << 28),
        BLUE     = (1u << 27),
        MAGENTA  = (1u << 26),
        CYAN     = (1u << 25),
        WHITE    = (1u << 24),
        num_bits = 8;
    } // namespace back

    static_assert(back::num_bits + fore::num_bits <= sizeof(value_type) * 8);
    static_assert(back::WHITE > fore::WHITE);

    /// Sets stdout foreground and background color.
    void set(unsigned int color);

    /// Wrapper for `printf` enabling colored printing.
    void printf(int color, char const *fmt, ...);

  } // namespace color

} // namespace term

#endif // NLUKA_TERM_HPP
