#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#if _WIN32
#include <Windows.h>
#undef max
#endif

#include "ant.hpp"
#include "exit.hpp"
#include "term.hpp"
#include "timespan.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace term::color;
using time_point = std::chrono::steady_clock::time_point;

time_point current_time() {
  return std::chrono::high_resolution_clock::now();
}

void update_ui(
  char const *const sim_name,
  u64 const generation_target,
  ant::simulation const &sim
) {
  static usize ticks = 0;
  time_point const start_time = current_time();

  while (true) {
    time_point const time_now = current_time();

    auto const nanos_elapsed = std::chrono::duration_cast<
      std::chrono::nanoseconds
    >(time_now - start_time);

    double const secs_elapsed = nanos_elapsed.count() / 1'000'000'000.0;
    u64 const gens_completed = sim.generations;
    double const mega_gens_completed = gens_completed / 1'000'000.0;
    double const mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed, 0.0 + DBL_EPSILON);
    double const percent_completion = (
      (gens_completed / static_cast<double>(generation_target)) * 100.0
    );

    u64 const gens_remaining = generation_target - gens_completed;
    double const mega_gens_remaining = gens_remaining / 1'000'000.0;
    double const secs_remaining = mega_gens_remaining / mega_gens_per_sec;

    bool const is_simulation_done =
      sim.last_step_res > ant::step_result::SUCCESS ||
      gens_completed >= generation_target
    ;

    // line 1, print at beginning and end
    if (ticks == 0 || is_simulation_done) {
      printf(fore::CYAN | back::BLACK, "%s", sim_name);
      printf(fore::DEFAULT | back::BLACK, " - ");
      if (gens_remaining > 0) {
        printf(fore::YELLOW | back::BLACK, "in progress");
      } else {
        printf(fore::DEFAULT | back::BLACK, "finished, %s", [&sim]() {
          switch (sim.last_step_res) {
            case ant::step_result::NIL : return "internal error (NIL step result)";
            case ant::step_result::SUCCESS : return "generation target reached";
            case ant::step_result::FAILED_AT_BOUNDARY : return "hit edge";
            default : return "(nullptr)";
          }
        }());
      }
      term::clear_to_end_of_line();
      putc('\n', stdout);
    } else {
      term::cursor::move_down(1);
    }

    // line 2, print every 1/10 seconds
    {
      printf(
        fore::DEFAULT | back::BLACK,
        "[%llu / %llu] %.1lf%%",
        gens_completed, generation_target, percent_completion
      );
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line 3, print every 1 seconds
    if (ticks % 10 == 0) {
      printf(fore::DEFAULT | back::BLACK, "%.2lf Mgens/sec", mega_gens_per_sec);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    } else {
      term::cursor::move_down(1);
    }

    // line 4, print every 1/10 seconds
    {
      printf(
        fore::DEFAULT | back::BLACK,
        "%s elapsed",
        timespan_to_string(timespan_calculate(static_cast<usize>(secs_elapsed))).c_str()
      );
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    if (is_simulation_done) {
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    term::cursor::move_up(4);
  }
}

int main(int const argc, char const *const *const argv) {
  if (argc != 3 + 1 /* executable pathname */) {
    using namespace term::color;
    printf(
      fore::RED | back::BLACK,
      "usage: <name> <sim_file> <generation_target> [<img_fmt>]\n"
      "  <name> = name of simulation\n"
      "  <sim_file> = pathname to simulation JSON file\n"
      "  <generation_target> = uint64 in range [1, UINT64_MAX]\n"
      // "  <img_fmt> = PGM image format, plain|raw (raw is faster)\n"
    );
    EXIT(exit_code::WRONG_NUM_OF_ARGS);
  }

  char const
    *const sim_name = argv[1],
    *const sim_file_pathname = argv[2],
    *const generation_target_str = argv[3]//,
    // *const img_fmt_str = argv[4]
  ;

  auto const current_dir = std::filesystem::current_path();

  try {
    auto parse_result = ant::simulation_parse(
      util::extract_txt_file_contents(sim_file_pathname),
      current_dir
    );

    ant::simulation &sim = parse_result.first;
    std::vector<std::string> const &errors = parse_result.second;

    if (!errors.empty()) {
      using namespace term::color;
      printf(fore::RED | back::BLACK, "fatal: malformed simulation file (%zu errors)\n", errors.size());
      for (auto const &err : errors)
        printf(fore::RED | back::BLACK, "  %s\n", err.c_str());
      EXIT(exit_code::BAD_SIM_FILE);
    }

    u64 const generation_target = std::stoull(generation_target_str);

    std::thread sim_thread(ant::simulation_run,
      std::ref(sim),
      sim_name,
      generation_target,
      pgm8::format::RAW,
      current_dir
    );

    #if _WIN32
    SetPriorityClass(sim_thread.native_handle(), HIGH_PRIORITY_CLASS);
    #endif

    std::thread ui_update_thread(update_ui,
      sim_name, generation_target, std::ref(sim)
    );

    term::cursor::hide();
    ui_update_thread.join();
    sim_thread.join();
    term::cursor::show();

    EXIT(exit_code::SUCCESS);

  } catch (std::invalid_argument const &err) {

    printf(fore::RED | back::BLACK, "fatal: %s\n", err.what());
    EXIT(exit_code::BAD_ARG_VALUE);

  } catch (std::runtime_error const &err) {

    printf(fore::RED | back::BLACK, "fatal: %s\n", err.what());
    EXIT(exit_code::GENERIC_ERROR);

  } catch (...) {

    printf(fore::RED | back::BLACK, "fatal: unknown error occurred\n");
    EXIT(exit_code::UNKNOWN_ERROR);

  }
}
