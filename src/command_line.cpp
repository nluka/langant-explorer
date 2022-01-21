#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <stdio.h>
#include <filesystem>
#include "command_line.hpp"
#include "CommandVars.hpp"
#include "constants.hpp"
#include "ui/print.hpp"
#include "util/util.hpp"

void validate_option_value_exists(char const * argName, int argIndex, int argc);

void process_command_line_args(int const argc, char const * const * const argv) {
  if (argc < 2) {
    printfc(TextColor::Red, "%s\n", "Missing simulation file pathname as first argument");
    end(ExitCode::MissingSimulationFile);
  }

  // handle help/version options
  for (int i = 1; i < argc; ++i) {
    char const * const arg = argv[i];
    if (
      are_strings_identical(arg, "-h") ||
      are_strings_identical(arg, "--help")
    ) {
      printf("usage: SIMULATION_FILE_PATHNAME [OPTION]...\n");
      printf("options:\n");
      printf("  -o, --image-output-path  specifies where to save image files [default: cwd]\n");
      printf("  -l, --log                specifies pathname to which event log will be saved [default: none]\n");
      printf("  -n, --no-image-output    disables image output\n");
      end(ExitCode::Success);
    } else if (
      are_strings_identical(arg, "-v") ||
      are_strings_identical(arg, "--version")
    ) {
      printf("version 1.0.0\n");
      end(ExitCode::Success);
    }
  }

  { // process SIMULATION_FILE_PATHNAME
    g_comVars.simulationFilePathname = argv[1];
    string_fix_path_separators(g_comVars.simulationFilePathname);
  }

  // process options
  for (int i = 2; i < argc; ++i) {
    char const * const option = argv[i];
    if (
      are_strings_identical(option, "-o") ||
      are_strings_identical(option, "--image-output-path")
    ) {
      validate_option_value_exists(option, i, argc);
      g_comVars.imageFileOutputPath = argv[i + 1];
      string_fix_path_separators(g_comVars.imageFileOutputPath);
      if (g_comVars.imageFileOutputPath.back() != std::filesystem::path::preferred_separator) {
        g_comVars.imageFileOutputPath.push_back(std::filesystem::path::preferred_separator);
      }
      ++i;
    } else if (
      are_strings_identical(option, "-l") ||
      are_strings_identical(option, "--log")
    ) {
      validate_option_value_exists(option, i, argc);
      g_comVars.eventLogFilePathname = argv[i + 1];
      string_fix_path_separators(g_comVars.eventLogFilePathname);
      ++i;
    } else if (
      are_strings_identical(option, "-n") ||
      are_strings_identical(option, "--no-image-output")
    ) {
      g_comVars.outputImageFiles = false;
    } else {
      printfc(TextColor::Red, "Unknown option %s\n", option);
      end(ExitCode::UnknownArgument);
    }
  }
}

void validate_option_value_exists(char const * const argName, int const argIndex, int const argc) {
  if (argIndex >= argc - 1) {
    printfc(TextColor::Red, "Option %s missing value\n", argName);
    end(ExitCode::ArgumentMissingValue);
  }
}
