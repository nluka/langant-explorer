#include "util.hpp"
#include "../ui/print.hpp"
#include "../CommandVars.hpp"

void end(ExitCode const code, char const * const message) {
  if (message != NULL) {
    printfc(TextColor::Magenta, "%s, ", message);
  }
  printfc(TextColor::Magenta, "exiting...\n\n");

  stdout_cursor_show();
  exit(static_cast<int>(code));
}
