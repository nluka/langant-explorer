#ifndef EXIT_HPP
#define EXIT_HPP

#include <cstdlib>

enum class exit_code : int {
  UNKNOWN_ERROR,
  GENERIC_ERROR,
  SUCCESS = 0,
  INVALID_OPTION_SYNTAX,
  MISSING_REQUIRED_OPTION,
  BAD_OPTION_VALUE,
  FILE_NOT_FOUND,
  FILE_OPEN_FAILED,
  BAD_FILE,
  BAD_SCENARIO,
};

#define EXIT(code) \
std::exit(static_cast<int>(code))

#endif // EXIT_HPP
