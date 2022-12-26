#ifndef ANT_HPP
#define ANT_HPP

#include <array>
#include <cinttypes>
#include <vector>
#include <string>

#include "pgm8.hpp"

namespace ant {

// not using an enum class because I want to do arithmetic with
// turn_direction values without casting.
namespace orientation {
  typedef int8_t type;

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
  typedef int8_t type;

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
  typedef int8_t type;

  type const
    NIL = -1,
    SUCCESS = 0,
    FAILED_AT_BOUNDARY = 1;

  char const *to_string(type);
};

struct rule {
  uint8_t replacement_shade = 0;
  turn_direction::type turn_dir = turn_direction::NIL;

  bool operator!=(rule const &) const noexcept;
  friend std::ostream &operator<<(std::ostream &, rule const &);
};

struct simulation {
  // state
  uint_fast64_t generations;
  step_result::type last_step_res;
  int grid_width, grid_height, ant_col, ant_row;
  int8_t ant_orientation;
  uint8_t *grid;

  std::array<rule, 256> rules;
  uint_fast64_t save_interval;
  size_t num_save_points;
  std::array<uint_fast64_t, 7> save_points;
};

typedef std::pair<simulation, std::vector<std::string>> simulation_parse_result_t;
simulation_parse_result_t simulation_parse(std::string const &);

step_result::type simulation_step_forward(simulation &);

void simulation_save(simulation const &, char const *name, pgm8::format);

} // namespace ant

#endif // ANT_HPP
