#include <thread>
#include "CommandVars.hpp"
#include "command_line.hpp"
#include "core/core.hpp"
#include "ui/print.hpp"
#include "util/util.hpp"

CommandVars g_comVars;

void print_simulation_statuses(
  Simulation const sims[],
  size_t const simCount
) {
  while (1) {
    size_t simsCompletedCount = 0, linesPrintedCount = 0;
    for (size_t i = 0; i < simCount; ++i) {
      if (simulation_is_finished(&sims[i])) {
        ++simsCompletedCount;
      }
      linesPrintedCount += simulation_print(&sims[i]);
    }
    if (simsCompletedCount == simCount) {
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    stdout_cursor_move_up(linesPrintedCount);
  }
}

int main(int const argc, char const * const * const argv) {
  stdout_cursor_hide();
  print_banner();
  process_command_line_args(argc, argv);
  prepare_event_log_file();

  Simulation sims[MAX_SIMULATION_COUNT] = { 0 };
  const size_t simCount = parse_simulation_file(sims);

  std::thread simThreads[MAX_SIMULATION_COUNT];
  for (size_t i = 0; i < simCount; ++i) {
    simulation_init(&sims[i]);
    simThreads[i] = std::thread(simulation_run, &sims[i]);
  }

  std::thread simStatusesPrinterThread(
    print_simulation_statuses, sims, simCount
  );
  simStatusesPrinterThread.join();
  for (size_t i = 0; i < simCount; ++i) {
    simThreads[i].join();
  }

  end(ExitCode::Success);
}
