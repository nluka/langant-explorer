#include <algorithm>
#include <cstdarg>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <variant>
#include <regex>

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include "json.hpp"
#include "term.hpp"
#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"
#include "logger.hpp"
#include "program_options.hpp"
#include "platform.hpp"

namespace fs = std::filesystem;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

static po::simulate_one_options s_options{};
static simulation::state s_sim_state{};
static simulation::run_result s_sim_run_result{};

void ui_loop(std::string const &sim_name);

i32 main(i32 const argc, char const *const *const argv) {
  if (argc < 2) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "Usage:\n"
      "  simulate_one [options]\n"
      "\n";
    po::simulate_one_options_description().print(usage_msg, 6);
    usage_msg << '\n';
    std::cout << usage_msg.str();
    return 1;
  }

  {
    errors_t errors{};

    po::parse_simulate_one_options(argc, argv, s_options, errors);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      return 1;
    }

    s_sim_state = simulation::parse_state(
      util::extract_txt_file_contents(s_options.state_file_path, false),
      std::filesystem::current_path(),
      errors);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      return 1;
    }
  }

  if (s_options.log_file_path != "") {
    logger::set_out_file_path(s_options.log_file_path);
    logger::set_autoflush(true);
  }

  std::string const &true_sim_name = s_options.name == ""
    ? simulation::extract_name_from_json_state_path(s_options.state_file_path)
    : s_options.name;

  std::thread sim_thread([&true_sim_name]() {
    s_sim_run_result = simulation::run(
      s_sim_state,
      true_sim_name,
      s_options.sim.generation_limit,
      s_options.sim.save_points,
      s_options.sim.save_interval,
      s_options.sim.image_format,
      s_options.sim.save_path,
      s_options.sim.save_final_state,
      s_options.sim.create_logs,
      s_options.sim.save_image_only);

    delete[] s_sim_state.grid;
  });

#if ON_WINDOWS
  SetPriorityClass(sim_thread.native_handle(), HIGH_PRIORITY_CLASS);
#endif

  term::hide_cursor();
  std::atexit(term::unhide_cursor);

  ui_loop(true_sim_name); // blocks until simulation completes
  sim_thread.join();

  return 0;
}

void ui_loop(std::string const &sim_name)
{
  using namespace term;
  using util::time_point_t;

  std::string const horizontal_rule(sim_name.length(), '-');

  time_point_t const start_time = util::current_time();

  for (;;) {
    time_point_t const time_now = util::current_time();
    auto const time_breakdown = simulation::query_activity_time_breakdown(s_sim_state, time_now);

    u64 const
      total_nanos_elapsed = util::nanos_between(start_time, time_now),
      current_generation = s_sim_state.generation,
      gens_completed = s_sim_state.generations_completed(),
      gens_remaining = s_options.sim.generation_limit - gens_completed;
    f64 const
      total_secs_elapsed = total_nanos_elapsed / 1'000'000'000.0,
      secs_elapsed_iterating = time_breakdown.nanos_spent_iterating / 1'000'000'000.0,
      mega_gens_completed = gens_completed / 1'000'000.0,
      mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed_iterating, 0.0 + std::numeric_limits<f64>::epsilon()),
      percent_completion = ((gens_completed / gens_remaining) * 100.0),
      percent_iteration = ( time_breakdown.nanos_spent_iterating / (static_cast<f64>(time_breakdown.nanos_spent_iterating + time_breakdown.nanos_spent_saving)) ) * 100.0,
      percent_saving    = ( time_breakdown.nanos_spent_saving    / (static_cast<f64>(time_breakdown.nanos_spent_iterating + time_breakdown.nanos_spent_saving)) ) * 100.0;
    b8 const is_simulation_done = s_sim_run_result.code != simulation::run_result::code::NIL;

    u64 num_lines_printed = 0;

    auto const print_line = [&num_lines_printed](std::function<void ()> const &func) {
      func();
      term::clear_to_end_of_line();
      putc('\n', stdout);
      ++num_lines_printed;
    };

    print_line([&] {
      printf("Name       : ");
      printf(FG_CYAN, "%s", sim_name.c_str());
      term::clear_to_end_of_line();
      putc('\n', stdout);
    });

    print_line([&] {
      printf("Result     : %s",
        [&] {
          if (!is_simulation_done) {
            return "TBD";
          } else {
            switch (s_sim_state.last_step_res) {
              default:
              case simulation::step_result::NIL:      return "nil step result";
              case simulation::step_result::SUCCESS:  return "reached generation limit";
              case simulation::step_result::HIT_EDGE: return "hit edge";
            }
          }
        }()
      );

    });

    print_line([&] {
      printf("Mgens/sec  : ");
      printf(FG_MAGENTA, "%.2lf", mega_gens_per_sec);
    });

    print_line([&] {
      printf("Completion : ");
      printf(FG_BRIGHT_GREEN, "%.1lf %%", std::isinf(percent_completion) ? 0.0 : percent_completion);
    });

    print_line([&] {
      printf("Generation : %zu", current_generation);
    });

    print_line([&] {
      printf("Elapsed    : %s", util::time_span(static_cast<u64>(total_secs_elapsed)).to_string().c_str());
    });

    print_line([&] {
      printf("I/S Ratio  : %.1lf / %.1lf",
        std::isnan(percent_iteration) ? 0.0 : percent_iteration,
        std::isnan(percent_saving)    ? 0.0 : percent_saving);
    });

    print_line([&] {
      printf("Activity   : %s", [] {
        switch (s_sim_state.current_activity) {
          default:
          case simulation::activity::NIL:       return "nil";
          case simulation::activity::ITERATING: return "iterating";
          case simulation::activity::SAVING:    return "saving";
        }
      }());
    });

    if (is_simulation_done)
      return;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    term::move_cursor_up(num_lines_printed);
  }
}
