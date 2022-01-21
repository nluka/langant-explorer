#ifndef ANT_SIMULATOR_LITE_COMMAND_VARS_HPP
#define ANT_SIMULATOR_LITE_COMMAND_VARS_HPP

#include <stdbool.h>
#include <string>

struct CommandVars {
  bool        outputImageFiles = true;
  std::string simulationFilePathname{};
  std::string imageFileOutputPath{};
  std::string eventLogFilePathname{};
};

// command variables
extern CommandVars g_comVars;

#endif // ANT_SIMULATOR_LITE_COMMAND_VARS_HPP
