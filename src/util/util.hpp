#ifndef ANT_SIMULATOR_LITE_UTIL_UTIL_HPP
#define ANT_SIMULATOR_LITE_UTIL_UTIL_HPP

#include <stdio.h>
#include <inttypes.h>
#include <string>
#include <fstream>
#include "Timespan.hpp"

enum class ExitCode {
  Success = 0,
  MissingSimulationFile,
  UnknownArgument,
  ArgumentMissingValue,
  FailedToAllocateMemory,
  FailedToOpenFile,
  FailedToParseSimulationFile
};

void end(ExitCode code, char const * message = NULL);

size_t get_one_dimensional_index(
  size_t arrWidth,
  size_t targetColumn,
  size_t targetRow
);

bool are_strings_identical(char const * str1, char const * str2);

void string_fix_path_separators(std::string & str);

void assert_file(
  std::ofstream const & fileHandle,
  char const * filePathname,
  bool shouldTerminateOnBadHandle
);
void assert_file(
  FILE const * fileHandle,
  char const * filePathname,
  bool shouldTerminateOnBadHandle
);

#endif // ANT_SIMULATOR_LITE_UTIL_UTIL_HPP
