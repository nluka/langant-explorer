#ifndef ANT_SIMULATOR_LITE_CORE_RULESET_HPP
#define ANT_SIMULATOR_LITE_CORE_RULESET_HPP

#include <inttypes.h>
#include "Rule.hpp"
#include "Grid.hpp"

struct Ruleset {
  Rule rules[256];
};

void    ruleset_validate(Ruleset const * ruleset, Grid const * const grid, char const * simName);
color_t ruleset_find_largest_color(Ruleset const * ruleset);

#endif // ANT_SIMULATOR_LITE_CORE_RULESET_HPP
