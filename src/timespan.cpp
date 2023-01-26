#include <sstream>

#include "timespan.hpp"

timespan timespan_calculate(time_t secs) {
  usize const
    SECONDS_PER_DAY = 86400,
    SECONDS_PER_HOUR = 3600,
    SECONDS_PER_MINUTE = 60;

  usize
    days = 0,
    hours = 0,
    minutes = 0,
    seconds = static_cast<usize>(secs);

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

std::string timespan_to_string(timespan const &ts) {
  std::stringstream ss;

  if (ts.days > 0) {
    ss << ts.days << "d ";
  }
  if (ts.hours > 0) {
    ss << ts.hours << "h ";
  }
  if (ts.minutes > 0) {
    ss << ts.minutes << "m ";
  }
  ss << ts.seconds << 's';

  return ss.str();
}
