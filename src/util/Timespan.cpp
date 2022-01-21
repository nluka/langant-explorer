#include <sstream>
#include "Timespan.hpp"
#include "util.hpp"

Timespan timespan_calculate(time_t const secondsElapsed) {
  static size_t const
    SECONDS_PER_DAY = 86400,
    SECONDS_PER_HOUR = 3600,
    SECONDS_PER_MINUTE = 60;

  size_t
    days = 0,
    hours = 0,
    minutes = 0,
    seconds = static_cast<size_t>(secondsElapsed);

  while (seconds >= SECONDS_PER_DAY) {
    ++days;
    seconds -= SECONDS_PER_DAY;
  }

  while (seconds >= SECONDS_PER_HOUR) {
    ++hours;
    seconds -= SECONDS_PER_HOUR;
  }

  while (seconds >= SECONDS_PER_MINUTE) {
    ++minutes;
    seconds -= SECONDS_PER_MINUTE;
  }

  return { days, hours, minutes, seconds };
}

std::string timespan_convert_to_string(Timespan const * const timespan) {
  std::stringstream ss;

  if (timespan->days > 0) {
    ss << timespan->days << "d ";
  }
  if (timespan->hours > 0) {
    ss << timespan->hours << "h ";
  }
  if (timespan->minutes > 0) {
    ss << timespan->minutes << "m ";
  }
  ss << timespan->seconds << 's';

  return ss.str();
}
