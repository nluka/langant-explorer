#include <algorithm>
#include <cstdarg>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <variant>
#include <regex>
#include <mutex>
#include <condition_variable>

#include "util.hpp"
#include "term.hpp"
#include "program_options.hpp"
#include "simulation.hpp"
#include "logger.hpp"

#if ON_WINDOWS
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

namespace fs = std::filesystem;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

static po::simulate_one_options s_options{};
static simulation::state s_sim_state{};
static simulation::run_result s_sim_run_result{};

i32 main(i32 const argc, char const *const *const argv)
try
{
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

  // parse initial simulation state
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
    logger::set_stdout_logging(true);
  }

  std::string const &true_sim_name = s_options.name == ""
    ? simulation::extract_name_from_json_state_path(s_options.state_file_path)
    : s_options.name;

  std::condition_variable progress_sleep_cv{};
  std::mutex progress_sleep_mutex{};
  bool sim_finished = false;

  std::thread sim_thread([&] {
    std::atomic<u64> num_simulations_processed(0);

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
      s_options.sim.save_image_only,
      &num_simulations_processed,
      1
    );

    delete[] s_sim_state.grid;

    std::lock_guard sleep_lock(progress_sleep_mutex);
    sim_finished = true;
    progress_sleep_cv.notify_one();
  });

  util::set_thread_priority_high(sim_thread);

  term::hide_cursor();
  std::atexit(term::unhide_cursor);

  while (true) {
    b8 const is_simulation_done = s_sim_run_result.code != simulation::run_result::code::NIL;
    if (is_simulation_done) {
      break;
    }

    std::unique_lock sleep_lock(progress_sleep_mutex);

    if (progress_sleep_cv.wait_for(sleep_lock, std::chrono::milliseconds(5000), [&] { return sim_finished; })) {
      // task finished
    } else {
      // wait timed out

      if (s_options.sim.create_logs) {
        using logger::log;
        using logger::event_type;
        using logger::MAX_SIM_NAME_DISPLAY_LEN;

        u64 const
          gens_completed = s_sim_state.generations_completed(),
          gens_remaining = s_options.sim.generation_limit - gens_completed;
        f64 const
          mega_gens_per_sec = s_sim_state.compute_mega_gens_per_sec(),
          percent_completion = ( ( f64(gens_completed) / f64(gens_completed + gens_remaining) ) * 100.0 );

        log(event_type::SIM_PROGRESS, "%*.*s | %6.2lf %%, %zu, %6.2lf Mgens/s",
          MAX_SIM_NAME_DISPLAY_LEN, MAX_SIM_NAME_DISPLAY_LEN, true_sim_name.c_str(),
          percent_completion, gens_completed, mega_gens_per_sec);
      }
    }
  }

  std::exit(0);
}
catch (std::exception const &except)
{
  die("%s", except.what());
}
catch (...)
{
  die("unknown error - catch (...)");
}
