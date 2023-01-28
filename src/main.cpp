#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ant.hpp"
#include "exit.hpp"
#include "prgopts.hpp"
#include "term.hpp"
#include "timespan.hpp"
#include "types.hpp"
#include "util.hpp"

#ifdef MICROSOFT_COMPILER
# include <Windows.h>
# undef max
#endif

using namespace term::color;
using time_point = std::chrono::steady_clock::time_point;

time_point current_time() {
  return std::chrono::high_resolution_clock::now();
}

void update_ui(
  std::string const sim_name_,
  u64 const generation_target,
  ant::simulation const &sim
) {
  char const *const sim_name = sim_name_.c_str();
  static usize ticks = 0;
  time_point const start_time = current_time();

  while (true) {
    time_point const time_now = current_time();
    auto const nanos_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(time_now - start_time);
    f64 const secs_elapsed = nanos_elapsed.count() / 1'000'000'000.0;
    u64 const gens_completed = sim.generations;
    f64 const mega_gens_completed = gens_completed / 1'000'000.0;
    f64 const mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed, 0.0 + DBL_EPSILON);
    f64 const percent_completion = ((gens_completed / static_cast<f64>(generation_target)) * 100.0);
    u64 const gens_remaining = generation_target - gens_completed;
    f64 const mega_gens_remaining = gens_remaining / 1'000'000.0;
    f64 const secs_remaining = mega_gens_remaining / mega_gens_per_sec;

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
        "%llu generations, %.1lf%%",
        gens_completed, percent_completion
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

namespace bpo = boost::program_options;

template <typename Ty>
Ty get_required_option(
  char const *const optname,
  bpo::variables_map const &opts
) {
  if (opts.count(optname) == 0) {
    std::cerr << "missing required option --" << optname;
    EXIT(exit_code::MISSING_REQUIRED_OPTION);
  } else {
    return opts.at(optname).as<Ty>();
  }
}

int main(int const argc, char const *const *const argv) {
  if (argc == 1) {
    puts(prgopts::usage_msg());
    EXIT(exit_code::MISSING_REQUIRED_OPTION);
  }

  // parse command line args
  bpo::variables_map options;
  try {
    bpo::store(
      bpo::parse_command_line(argc, argv, prgopts::options_descrip()),
      options
    );
  } catch (std::exception const &err) {
    std::cerr << "fatal: " << err.what() << '\n';
    EXIT(exit_code::INVALID_OPTION_SYNTAX);
  }
  bpo::notify(options);

  std::string const sim_name = get_required_option<std::string>("name", options);
  std::string const sim_file_pathname = get_required_option<std::string>("scenario", options);
  u64 const generation_target = get_required_option<u64>("gentarget", options);

  pgm8::format const img_fmt = [&options]() {
    if (options.count("imgfmt") == 0) {
      return pgm8::format::RAW;
    } else {
      std::string const fmt = options.at("imgfmt").as<std::string>();
      if (fmt == "rawPGM") {
        return pgm8::format::RAW;
      } else if (fmt == "plainPGM") {
        return pgm8::format::PLAIN;
      } else {
        return pgm8::format::NIL;
      }
    }
  }();
  if (img_fmt == pgm8::format::NIL) {
    std::cerr << "fatal: invalid --imgfmt, must be rawPGM|plainPGM\n";
    EXIT(exit_code::BAD_OPTION_VALUE);
  }

  try {
    auto parse_result = ant::simulation_parse(
      util::extract_txt_file_contents(sim_file_pathname.c_str()),
      std::filesystem::current_path()
    );

    ant::simulation &sim = parse_result.first;
    std::vector<std::string> const &errors = parse_result.second;

    if (!errors.empty()) {
      using namespace term::color;
      printf(fore::RED | back::BLACK, "fatal: malformed simulation file (%zu errors)\n", errors.size());
      for (auto const &err : errors)
        printf(fore::RED | back::BLACK, "  %s\n", err.c_str());
      EXIT(exit_code::BAD_SCENARIO);
    }

    std::thread sim_thread(ant::simulation_run,
      std::ref(sim),
      sim_name,
      generation_target,
      img_fmt,
      std::filesystem::current_path()
    );

    #ifdef MICROSOFT_COMPILER
    SetPriorityClass(sim_thread.native_handle(), HIGH_PRIORITY_CLASS);
    #endif

    std::thread ui_update_thread(update_ui,
      sim_name,
      generation_target,
      std::ref(sim)
    );

    term::cursor::hide();
    ui_update_thread.join();
    sim_thread.join();
    term::cursor::show();

    EXIT(exit_code::SUCCESS);

  } catch (std::invalid_argument const &err) {

    printf(fore::RED | back::BLACK, "fatal: %s\n", err.what());
    EXIT(exit_code::BAD_OPTION_VALUE);

  } catch (std::runtime_error const &err) {

    printf(fore::RED | back::BLACK, "fatal: %s\n", err.what());
    EXIT(exit_code::GENERIC_ERROR);

  } catch (...) {

    printf(fore::RED | back::BLACK, "fatal: unknown error occurred\n");
    EXIT(exit_code::UNKNOWN_ERROR);

  }
}
