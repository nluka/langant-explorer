#ifndef ANT_HPP
#define ANT_HPP

#include <array>
#include <cinttypes>
#include <vector>
#include <string>

#include "pgm8.hpp"
#include "types.hpp"

namespace ant {

// not using an enum class because I want to do arithmetic with
// turn_direction values without casting.
namespace orientation {
  typedef i8 type;

  type const
    OVERFLOW_COUNTER_CLOCKWISE = 0,
    NORTH = 1,
    EAST = 2,
    SOUTH = 3,
    WEST = 4,
    OVERFLOW_CLOCKWISE = 5;

  char const *to_string(type);
}

// not using an enum class because I want to do arithmetic with
// orientation values without casting.
namespace turn_direction {
  typedef i8 type;

  type const
    NIL = -2, // represents no value
    LEFT = -1,
    NONE = 0, // means "don't turn"
    RIGHT = 1;

  char const *to_string(type);
}

// not using an enum class because the other 2 enumerated types are done with
// namespaces, so let's stay consistent.
namespace step_result {
  typedef i8 type;

  type const
    NIL = -1,
    SUCCESS = 0,
    FAILED_AT_BOUNDARY = 1;

  char const *to_string(type);
};

struct rule {
  u8 replacement_shade = 0;
  turn_direction::type turn_dir = turn_direction::NIL;

  bool operator!=(rule const &) const noexcept;
  friend std::ostream &operator<<(std::ostream &, rule const &);
};

struct simulation {
  // state
  u64 generations;
  step_result::type last_step_res;
  i32 grid_width, grid_height, ant_col, ant_row;
  i8 ant_orientation;
  u8 *grid;

  std::array<rule, 256> rules;
  u64 save_interval;
  usize num_save_points;
  std::array<u64, 7> save_points;
};

typedef std::pair<
  simulation, std::vector<std::string>
> simulation_parse_result_t;

simulation_parse_result_t simulation_parse(
  std::string const &json_str,
  std::filesystem::path const &dir
);

step_result::type simulation_step_forward(simulation &sim);

void simulation_save(
  simulation const &sim,
  char const *name,
  std::filesystem::path const &dir,
  pgm8::format img_fmt
);

void simulation_run(
  simulation &sim,
  char const *name,
  u64 generation_target,
  pgm8::format img_fmt,
  std::filesystem::path const &save_dir
);

} // namespace ant

#endif // ANT_HPP
