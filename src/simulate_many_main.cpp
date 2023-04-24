#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>

#include <boost/program_options.hpp>
#include "lib/BS_thread_pool_light.hpp"
#include "lib/json.hpp"
#include "lib/term.hpp"
#include "lib/fregex.hpp"

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"
#include "logger.hpp"
#include "program_options.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::time_point_t;
using util::errors_t;
using util::print_err;
using util::die;

static po::simulate_many_options s_options{};
static std::atomic<u64> s_num_simulations_done(0);
static std::atomic<u64> s_num_simulations_in_progress(0);
static std::atomic<u64> s_num_simulations_failed(0);

struct named_simulation
{
  std::string name;
  simulation::state state;
};

void ui_loop(std::vector<named_simulation> const &);

i32 main(i32 const argc, char const *const *const argv)
{
  if (argc < 2) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "Usage:\n"
      "  simulate_many [options]\n"
      "\n";
    po::simulate_many_options_description().print(usage_msg, 6);
    usage_msg << '\n';
    std::cout << usage_msg.str();
    return 1;
  }

  {
    errors_t errors{};
    po::parse_simulate_many_options(argc, argv, s_options, errors);

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

  std::vector<fs::path> const state_files = fregex::find(
    s_options.state_dir_path.c_str(),
    ".*\\.json",
    fregex::entry_type::regular_file);

  std::vector<named_simulation> simulations(state_files.size());

  // parse all state files in provided path and register them for simulation
  for (u64 i = 0; i < simulations.size(); ++i) {
    auto const &state_file_path = state_files[i];
    std::string const path_str = state_file_path.generic_string();

    errors_t errors{};

    simulations[i].state = simulation::parse_state(
      util::extract_txt_file_contents(path_str.c_str(), false),
      fs::path(s_options.state_dir_path),
      errors);

    if (!errors.empty()) {
      print_err("in '%s':", path_str.c_str());
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      die("%zu state errors", errors.size());
    }

    std::string name = simulation::extract_name_from_json_state_path(state_file_path.filename().string());
    simulations[i].name = std::move(name);
  }

  auto const simulation_task = [](named_simulation &sim) {
    ++s_num_simulations_in_progress;

    try {
      [[maybe_unused]] auto const result = simulation::run(
        sim.state,
        sim.name,
        s_options.sim.generation_limit,
        s_options.sim.save_points,
        s_options.sim.save_interval,
        s_options.sim.image_format,
        s_options.sim.save_path,
        s_options.sim.save_final_state,
        s_options.sim.create_logs,
        s_options.sim.save_image_only
      );

      ++s_num_simulations_done;

    } catch (...) {
      ++s_num_simulations_failed;
    }

    --s_num_simulations_in_progress;
    delete[] sim.state.grid;
  };

  BS::thread_pool_light t_pool(s_options.num_threads);

#if _WIN32
  // I would const this, but native_handle() is not a const member function for some reason
  std::thread *threads = t_pool.get_threads_uniqueptr().get();

  for (u64 i = 0; i < t_pool.get_thread_count(); ++i) {
    SetPriorityClass(threads[i].native_handle(), HIGH_PRIORITY_CLASS);
  }
#endif

  for (auto &sim : simulations) {
    t_pool.push_task(simulation_task, std::ref(sim));
  }

  term::cursor::hide();
  std::atexit(term::cursor::show);

  ui_loop(simulations);

  return static_cast<int>(s_num_simulations_failed.load());
}

void ui_loop(std::vector<named_simulation> const &simulations)
{
  using namespace term::color;

  std::string const horizontal_rule(30, '-');

  time_point_t const start_time = util::current_time();

  for (;;) {
    time_point_t const time_now = util::current_time();

    u64
      gens_completed = 0,
      nanos_spent_iterating = 0,
      nanos_spent_saving = 0;
    for (auto const &sim : simulations) {
      gens_completed += sim.state.generations_completed();
      auto const time_breakdown = simulation::query_activity_time_breakdown(sim.state, time_now);
      nanos_spent_iterating += time_breakdown.nanos_spent_iterating;
      nanos_spent_saving += time_breakdown.nanos_spent_saving;
    }

    u64 const
      num_sims_completed = s_num_simulations_done.load(),
      num_sims_failed = s_num_simulations_failed.load(),
      num_sims_in_progress = s_num_simulations_in_progress.load(),
      num_sims_remaining = simulations.size() - num_sims_completed,
      total_nanos_elapsed = util::nanos_between(start_time, time_now);
    f64 const
      total_secs_elapsed = total_nanos_elapsed / 1'000'000'000.0,
      secs_elapsed_iterating = nanos_spent_iterating / 1'000'000'000.0,
      mega_gens_completed = gens_completed / 1'000'000.0,
      mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed_iterating, 0.0 + std::numeric_limits<f64>::epsilon()),
      percent_sims_completed = ( static_cast<f64>(num_sims_completed) / simulations.size() ) * 100.0,
      percent_iteration = ( nanos_spent_iterating / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0,
      percent_saving    = ( nanos_spent_saving    / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0;

    u64 num_lines_printed = 0;

    auto const print_line = [&num_lines_printed](std::function<void ()> const &func) {
      func();
      term::clear_to_end_of_line();
      putc('\n', stdout);
      ++num_lines_printed;
    };

    print_line([&] {
      printf("Mgens/sec : ");
      printf(fore::WHITE | back::MAGENTA, "%.2lf", mega_gens_per_sec);
    });

    print_line([&] {
      printf("Completed : ");
      printf(fore::LIGHT_GREEN | back::BLACK, "%zu", num_sims_completed);
      printf(" (%.1lf %%)", percent_sims_completed);
    });

    print_line([&] {
      printf("Remaining : ");
      printf(fore::LIGHT_YELLOW | back::BLACK, "%zu", num_sims_remaining);
    });

    print_line([&] {
      printf("Active    : ");
      printf(fore::LIGHT_BLUE | back::BLACK, "%zu", num_sims_in_progress);
    });

    print_line([&] {
      printf("Failed    : ");
      printf(fore::LIGHT_RED | back::BLACK, "%zu", num_sims_failed);
    });

    print_line([&] {
      printf("Elapsed   : %s", util::time_span(static_cast<u64>(total_secs_elapsed)).to_string().c_str());
    });

    print_line([&] {
      printf("I/S Ratio : %.1lf / %.1lf",
        std::isnan(percent_iteration) ? 0.0 : percent_iteration,
        std::isnan(percent_saving)    ? 0.0 : percent_saving);
    });

    if (num_sims_completed == simulations.size()) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    term::cursor::move_up(num_lines_printed);
  }
}
