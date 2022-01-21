#ifndef ANT_SIMULATOR_LITE_CORE_RULE_HPP
#define ANT_SIMULATOR_LITE_CORE_RULE_HPP

#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include "types.hpp"

#define RTD_LEFT static_cast<int8_t>(-1)
#define RTD_NONE static_cast<int8_t>(0)
#define RTD_RIGHT static_cast<int8_t>(1)

struct Rule {
  bool    isUsed;
  color_t replacementColor;
  int8_t  turnDirection;
};

#endif // ANT_SIMULATOR_LITE_CORE_RULE_HPP
