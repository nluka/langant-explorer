#ifndef TIMESPAN_HPP
#define TIMESPAN_HPP

#include <cstdlib>
#include <ctime>
#include <string>

#include "primitives.hpp"

struct timespan {
  usize days, hours, minutes, seconds;
};

timespan timespan_calculate(usize secs);
std::string timespan_to_string(timespan const &ts);

#endif // TIMESPAN_HPP
