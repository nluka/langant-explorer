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
#include "lib/pgm8.hpp"

#include "primitives.hpp"
#include "util.hpp"

#define SIM_OPT_STATEPATH_FULL "statepath"
#define SIM_OPT_STATEPATH_SHORT "i"

#define SIM_OPT_GENLIM_FULL "genlim"
#define SIM_OPT_GENLIM_SHORT "g"

#define SIM_OPT_IMGFMT_FULL "imgfmt"
#define SIM_OPT_IMGFMT_SHORT "f"

#define SIM_OPT_SAVEFINALSTATE_FULL "savefinalstate"
#define SIM_OPT_SAVEFINALSTATE_SHORT "s"

#define SIM_OPT_SAVEPOINTS_FULL "savepoints"
#define SIM_OPT_SAVEPOINTS_SHORT "p"

#define SIM_OPT_SAVEINTERVAL_FULL "saveinterval"
#define SIM_OPT_SAVEINTERVAL_SHORT "v"

#define SIM_OPT_SAVEPATH_FULL "savepath"
#define SIM_OPT_SAVEPATH_SHORT "o"

#define SIM_OPT_SAVEIMGONLY_FULL "saveimgonly"
#define SIM_OPT_SAVEIMGONLY_SHORT "y"

#define SIM_OPT_LOGSAVEPOINTS_FULL "logsavepoints"
#define SIM_OPT_LOGSAVEPOINTS_SHORT "l"

namespace simulation
{
  // not using an enum class because I want to do arithmetic with
  // turn_direction values without casting.
  namespace orientation
  {
    typedef i8 value_type;

    value_type from_string(char const *);
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

    value_type from_char(char);
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

  enum activity : u8
  {
    NIL,
    ITERATING,
    SAVING,
  };

  struct activity_time_breakdown
  {
    u64 nanos_spent_iterating;
    u64 nanos_spent_saving;
  };

  struct state
  {
    u64 generation;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    u8 *grid;
    u64 nanos_spent_iterating;
    u64 nanos_spent_saving;
    util::time_point_t activity_start;
    util::time_point_t activity_end;
    activity current_activity;
    orientation::value_type ant_orientation;
    step_result::value_type last_step_res;
    rules_t rules;
    u8 maxval;

    b8 can_step_forward(u64 generation_target = 0) const noexcept;

    activity_time_breakdown query_activity_time_breakdown(
      util::time_point_t now = util::current_time()) const;
  };

  std::variant<state, util::errors_t> parse_state(
    std::string const &json_str,
    std::filesystem::path const &dir);

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
    u64 generation,
    step_result::value_type last_step_res,
    i32 grid_width,
    i32 grid_height,
    std::string const &grid_state,
    i32 ant_col,
    i32 ant_row,
    orientation::value_type ant_orientation,
    rules_t const &rules);

  struct run_result
  {
    enum code : u8
    {
      NIL = 0,
      REACHED_GENERATION_LIMIT,
      HIT_EDGE,
    };

    code code = code::NIL;
    usize num_save_points_successful = 0;
    usize num_save_points_failed = 0;
  };

  run_result run(
    state &state,
    std::string const &name,
    u64 generation_target,
    std::vector<u64> save_points,
    u64 save_interval,
    pgm8::format img_fmt,
    std::filesystem::path const &save_dir,
    b8 save_final_state,
    b8 log_save_points,
    b8 save_image_only);

  struct env_config
  {
    using fs_path = std::filesystem::path;

    u64               generation_limit;
    u64               save_interval;
    fs_path           state_path;
    fs_path           save_path;
    std::vector<u64>  save_points;
    pgm8::format      img_fmt;
    b8                save_final_state;
    b8                log_save_points;
    b8                save_image_only;
  };

  std::variant<env_config, util::errors_t> extract_env_config(
    int argc,
    char const *const *argv,
    char const *state_path_desc);

  boost::program_options::options_description env_options_description(char const *state_path_desc);
}

#endif // SIMULATION_HPP
