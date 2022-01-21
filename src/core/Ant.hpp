#ifndef ANT_SIMULATOR_LITE_CORE_ANT_HPP
#define ANT_SIMULATOR_LITE_CORE_ANT_HPP

#include <inttypes.h>
#include "types.hpp"
#include "Grid.hpp"
#include "Ruleset.hpp"

#define AO_OVERFLOW_COUNTER_CLOCKWISE static_cast<int8_t>(-1)
#define AO_NORTH static_cast<int8_t>(0)
#define AO_EAST static_cast<int8_t>(1)
#define AO_SOUTH static_cast<int8_t>(2)
#define AO_WEST static_cast<int8_t>(3)
#define AO_OVERFLOW_CLOCKWISE static_cast<int8_t>(4)

struct Ant {
  uint16_t col;
  uint16_t row;
  int8_t   orientation;
};

enum class AntStepResult {
  Ok,
  HitBoundary,
};

void          ant_validate(Ant const * ant, Grid const * grid, char const * simName);
AntStepResult ant_take_step(Ant * ant, Grid * grid, Ruleset const * ruleset);

#endif // ANT_SIMULATOR_LITE_CORE_ANT_HPP
