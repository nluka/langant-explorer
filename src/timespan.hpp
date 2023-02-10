#ifndef TIMESPAN_HPP
#define TIMESPAN_HPP

#pragma warning(push, 0)
#include <cstdlib>
#include <ctime>
#include <string>
#pragma warning(pop)

#include "primitives.hpp"

struct timespan {
  usize days, hours, minutes, seconds;
};

timespan timespan_calculate(usize secs);
std::string timespan_to_string(timespan const &ts);

#endif // TIMESPAN_HPP
