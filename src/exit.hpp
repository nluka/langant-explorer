#ifndef EXIT_HPP
#define EXIT_HPP

#include <cstdlib>

enum class exit_code : int {
  SUCCESS = 0,
  GENERIC_ERROR,
  UNKNOWN_ERROR,
  WRONG_NUM_OF_ARGS,
  BAD_ARG_VALUE,
  FILE_NOT_FOUND,
  FILE_OPEN_FAILED,
  BAD_FILE,
  BAD_SIM_FILE,
};

#define EXIT(code) \
std::exit(static_cast<int>(code))

#endif // EXIT_HPP
