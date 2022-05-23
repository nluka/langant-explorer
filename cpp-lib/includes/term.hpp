#ifndef CPPLIB_TERM_HPP
#define CPPLIB_TERM_HPP

namespace term { // stands for `terminal`

enum class ColorText {
  DEFAULT = 0,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  GRAY = 90,
  GREY = 90,
};

// Sets stdout text color.
void set_color_text(ColorText);

void cursor_hide();
void cursor_show();

void cursor_move_up(size_t);

void clear_curr_line();

/*
  Wrapper for printf allowing for colored printing. Sets stdout text color to
  `g_defaultColorText` after printing - use `set_color_text_default` to change
  it.
*/
void printf_colored(ColorText, char const *fmt, ...);
void set_color_text_default(ColorText color);

} // namespace term

#endif // CPPLIB_TERM_HPP
