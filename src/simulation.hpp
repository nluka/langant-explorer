#ifndef ANT_HPP
#define ANT_HPP

#include <array>
#include <cinttypes>
#include <filesystem>
#include <string>
#include <vector>

#include "pgm8.hpp"
#include "types.hpp"
#include "util.hpp"

namespace simulation {

  // not using an enum class because I want to do arithmetic with
  // turn_direction values without casting.
  namespace orientation {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const OVERFLOW_COUNTER_CLOCKWISE = 0, NORTH = 1, EAST = 2, SOUTH = 3, WEST = 4,
                     OVERFLOW_CLOCKWISE = 5;
  } // namespace orientation

  // not using an enum class because I want to do arithmetic with
  // orientation values without casting.
  namespace turn_direction {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const NIL = -2, LEFT = -1, NO_CHANGE = 0, RIGHT = 1;
  } // namespace turn_direction

  // not using an enum class because the other 2 enumerated types are done with
  // namespaces, so let's stay consistent.
  namespace step_result {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const NIL = -1, SUCCESS = 0, FAILED_AT_BOUNDARY = 1;
  } // namespace step_result

  struct rule {
    u8 replacement_shade = 0;
    turn_direction::value_type turn_dir = turn_direction::NIL;

    b8 operator!=(rule const &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, rule const &);
  };

  static_assert(sizeof(rule) == 2);

  struct configuration {
    u64 generation;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    u8 *grid;
    orientation::value_type ant_orientation;
    step_result::value_type last_step_res;
    std::array<rule, 256> rules;

    b8 can_step_forward(u64 generation_target = 0) const noexcept;
  };

  configuration parse_config(
    std::string const &json_str,
    std::filesystem::path const &dir,
    util::strvec_t &errors);

  step_result::value_type attempt_step_forward(configuration &cfg);

  void save_config(
    configuration const &cfg,
    const char *name,
    std::filesystem::path const &dir,
    pgm8::format img_fmt);

  void run(
    configuration &cfg,
    std::string name,
    u64 generation_target,
    std::vector<u64> save_points,
    u64 save_interval,
    pgm8::format img_fmt,
    std::filesystem::path const &save_dir,
    b8 save_final_cfg);

} // namespace simulation

#endif // ANT_HPP
