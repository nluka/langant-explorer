#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#pragma warning(push, 0)
#include <array>
#include <cinttypes>
#include <filesystem>
#include <string>
#include <vector>
#pragma warning(pop)

#include "../pgm8.hpp"
#include "../primitives.hpp"
#include "../util.hpp"

namespace simulation
{
  // not using an enum class because I want to do arithmetic with
  // turn_direction values without casting.
  namespace orientation
  {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const
      OVERFLOW_COUNTER_CLOCKWISE = 0,
      NORTH = 1,
      EAST = 2,
      SOUTH = 3,
      WEST = 4,
      OVERFLOW_CLOCKWISE = 5;
  }

  // not using an enum class because I want to do arithmetic with
  // orientation values without casting.
  namespace turn_direction
  {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const
      NIL = -2,
      LEFT = -1,
      NO_CHANGE = 0,
      RIGHT = 1;
  }

  // not using an enum class because the other 2 enumerated types are done with
  // namespaces, so let's stay consistent.
  namespace step_result
  {
    typedef i8 value_type;
    char const *to_string(value_type);
    value_type const
      NIL = -1,
      SUCCESS = 0,
      HIT_EDGE = 1;
  }

  struct rule
  {
    u8 replacement_shade = 0;
    turn_direction::value_type turn_dir = turn_direction::NIL;

    b8 operator!=(rule const &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, rule const &);
  };

  static_assert(sizeof(rule) == 2);

  typedef std::array<rule, 256> rules_t;

  struct state
  {
    u64 generation;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    u8 *grid;
    orientation::value_type ant_orientation;
    step_result::value_type last_step_res;
    rules_t rules;
    u8 maxval;

    b8 can_step_forward(u64 generation_target = 0) const noexcept;
  };

  state parse_state(
    std::string const &json_str,
    std::filesystem::path const &dir,
    util::strvec_t &errors);

  step_result::value_type attempt_step_forward(state &state);

  void save_state(
    state const &state,
    const char *name,
    std::filesystem::path const &dir,
    pgm8::format img_fmt);

  void run(
    state &state,
    std::string sim_name,
    u64 generation_target,
    std::vector<u64> save_points,
    u64 save_interval,
    pgm8::format img_fmt,
    std::filesystem::path const &save_dir,
    b8 save_final_state);
}

#endif // SIMULATION_HPP
