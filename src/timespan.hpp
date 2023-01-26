#ifndef TIMESPAN_HPP
#define TIMESPAN_HPP

#include <cstdlib>
#include <string>
#include <ctime>

#include "types.hpp"

struct timespan {
  usize days, hours, minutes, seconds;
};

timespan timespan_calculate(time_t secs);
std::string timespan_to_string(timespan const &ts);

#endif // TIMESPAN_HPP
