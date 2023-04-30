#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <array>
#include <cinttypes>
#include <filesystem>
#include <string>
#include <vector>
#include <variant>
#include <ostream>

#include <boost/program_options.hpp>

#include "pgm8.hpp"
#include "primitives.hpp"
#include "util.hpp"

namespace simulation
{
  // not using an enum class because I want to do arithmetic with
  // turn_direction values without casting.
  namespace orientation
  {
    typedef i8 value_type;

    value_type from_cstr(char const *);
    char const *to_cstr(value_type);

    value_type const OVERFLOW_COUNTER_CLOCKWISE = 0;
    value_type const NORTH = 1;
    value_type const EAST = 2;
    value_type const SOUTH = 3;
    value_type const WEST = 4;
    value_type const OVERFLOW_CLOCKWISE = 5;
  }

  // not using an enum class because I want to do arithmetic with
  // orientation values without casting.
  namespace turn_direction
  {
    typedef i8 value_type;

    value_type from_char(char);
    char const *to_cstr(value_type);

    value_type const NIL = -2;
    value_type const LEFT = -1;
    value_type const NO_CHANGE = 0;
    value_type const RIGHT = 1;
  }

  // not using an enum class because the other 2 enumerated types are done with
  // namespaces, so let's stay consistent.
  namespace step_result
  {
    typedef i8 value_type;

    char const *to_cstr(value_type);

    value_type const NIL = -1;
    value_type const SUCCESS = 0;
    value_type const HIT_EDGE = 1;
  }

  struct rule
  {
    u8 replacement_shade = 0;
    turn_direction::value_type turn_dir = turn_direction::NIL;

    b8 operator!=(rule const &) const noexcept;
    friend std::ostream &operator<<(std::ostream &, rule const &);
  };

  // make sure there's no padding
  static_assert(sizeof(rule) == 2);

  typedef std::array<rule, 256> rules_t;

  enum activity : u8
  {
    NIL,
    ITERATING,
    SAVING,
  };

  struct state
  {
    u64 start_generation;
    u64 generation;
    u64 nanos_spent_iterating;
    u64 nanos_spent_saving;
    util::time_point_t activity_start;
    util::time_point_t activity_end;
    u8 *grid;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    activity current_activity;
    orientation::value_type ant_orientation;
    step_result::value_type last_step_res;
    u8 maxval;
    rules_t rules;

    b8 can_step_forward(u64 generation_limit = 0) const noexcept;
    u64 num_pixels() const noexcept;
    u64 generations_completed() const noexcept;
  };

  struct activity_time_breakdown
  {
    u64 nanos_spent_iterating;
    u64 nanos_spent_saving;
  };

  activity_time_breakdown query_activity_time_breakdown(
    state const &state,
    util::time_point_t now = util::current_time());

  state parse_state(
    std::string const &json_str,
    std::filesystem::path const &dir,
    util::errors_t &errors);

  step_result::value_type attempt_step_forward(state &state);

  u8 deduce_maxval_from_rules(rules_t const &rules);

  void save_state(
    state const &state,
    const char *name,
    std::filesystem::path const &dir,
    pgm8::format img_fmt,
    b8 image_only);

  void print_state_json(
    std::ostream &os,
    std::string const &grid_state,
    u64 generation,
    i32 grid_width,
    i32 grid_height,
    i32 ant_col,
    i32 ant_row,
    step_result::value_type last_step_res,
    orientation::value_type ant_orientation,
    u64 maxval_digits,
    rules_t const &rules);

  std::string extract_name_from_json_state_path(std::string const &);

  struct run_result
  {
    enum class code : u8
    {
      NIL = 0,
      REACHED_GENERATION_LIMIT,
      HIT_EDGE,
    };

    u64 num_save_points_successful = 0;
    u64 num_save_points_failed = 0;
    code code = code::NIL;
  };

  run_result run(
    state &state,
    std::string const &name,
    u64 generation_limit,
    std::vector<u64> save_points,
    u64 save_interval,
    pgm8::format img_fmt,
    std::filesystem::path const &save_dir,
    b8 save_final_state,
    b8 create_logs,
    b8 save_image_only,
    std::atomic<u64> *num_simulations_processed = nullptr,
    u64 total = 0);
}

#endif // SIMULATION_HPP
