#ifndef EXIT_HPP
#define EXIT_HPP

#pragma warning(push, 0)
#include <cstdlib>
#pragma warning(pop)

enum class exit_code : int
{
  UNKNOWN_ERROR,
  GENERIC_ERROR,
  SUCCESS = 0,
  INVALID_OPTION_SYNTAX,
  MISSING_REQUIRED_ARGUMENT,
  BAD_ARGUMENT_VALUE,
  FILE_NOT_FOUND,
  FILE_OPEN_FAILED,
  BAD_FILE,
  BAD_CONFIG,
};

[[noreturn]] inline
void die(exit_code const code)
{
  std::exit(static_cast<int>(code));
}

#endif // EXIT_HPP
