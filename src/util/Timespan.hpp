#ifndef ANT_SIMULATOR_LITE_UTIL_TIMESPAN_HPP
#define ANT_SIMULATOR_LITE_UTIL_TIMESPAN_HPP

#include <stdlib.h>
#include <time.h>
#include <string>

struct Timespan {
  size_t days;
  size_t hours;
  size_t minutes;
  size_t seconds;
};

Timespan timespan_calculate(time_t secondsElapsed);
std::string timespan_convert_to_string(Timespan const * timespan);

#endif // ANT_SIMULATOR_LITE_UTIL_TIMESPAN_HPP
