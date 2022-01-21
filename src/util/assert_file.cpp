#include "util.hpp"
#include "../core/core.hpp"

void assert_file(
  std::ofstream const & file,
  char const * const filePathname,
  bool shouldTerminateOnBadHandle
) {
  if (file.is_open()) {
    return;
  }

  log_event(
    shouldTerminateOnBadHandle ? EventType::Error : EventType::Warning,
    "Failed to open file '%s'",
    filePathname
  );

  if (shouldTerminateOnBadHandle) {
    end(ExitCode::FailedToOpenFile);
  }
}

void assert_file(
  FILE const * const file,
  char const * const filePathname,
  bool shouldTerminateOnBadHandle
) {
  if (file != NULL) {
    return;
  }

  log_event(
    shouldTerminateOnBadHandle ? EventType::Error : EventType::Warning,
    "Failed to open file '%s'",
    filePathname
  );

  if (shouldTerminateOnBadHandle) {
    end(ExitCode::FailedToOpenFile);
  }
}
