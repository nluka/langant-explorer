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
#include "lib/regexglob.hpp"

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include "primitives.hpp"
#include "timespan.hpp"
#include "util.hpp"
#include "simulation.hpp"
#include "logger.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::time_point_t;
using util::errors_t;
using util::print_err;
using util::die;

static simulation::env_config s_cfg{};
static std::atomic<usize> s_num_simulations_done(0);
static std::atomic<usize> s_num_simulations_in_progress(0);
static std::atomic<usize> s_num_simulations_failed(0);

struct named_simulation
{
  std::string name;
  simulation::state state;
};

std::string usage_msg();
void ui_loop(std::vector<named_simulation> const &);

int main(int const argc, char const *const *const argv)
{
  if (argc < 2) {
    std::cout << usage_msg();
    std::exit(1);
  }

  u32 num_threads = 0;
  try {
    num_threads = std::stoul(argv[1]);
  } catch (...) {
    die("unable to parse <num_threads>");
  }

  {
    char const *const log_path = argv[2];
    std::ofstream log_file(log_path);
    if (!log_file.is_open()) {
      die("unable to open <log_path> '%s'", log_path);
    }
    logger::set_out_pathname(log_path);
  }

  logger::set_autoflush(true);

  {
    std::variant<
      simulation::env_config,
      errors_t
    > extract_res = simulation::extract_env_config(argc, argv, "path to directory containing initial state .json files");

    if (std::holds_alternative<errors_t>(extract_res)) {
      errors_t const &errors = std::get<errors_t>(extract_res);
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      die("%zu configuration errors", errors.size());
    }

    s_cfg = std::move(std::get<simulation::env_config>(extract_res));

    // some additional validation specific to this program
    if (!fs::is_directory(s_cfg.state_path)) {
      die("(--" SIM_OPT_STATEPATH_FULL ", -" SIM_OPT_STATEPATH_SHORT ") path is not a directory");
    }
  }

  std::vector<fs::path> const state_files = regexglob::fmatch(
    s_cfg.state_path.string().c_str(),
    ".*\\.json"
  );

  if (state_files.empty()) {
    die("(--" SIM_OPT_STATEPATH_FULL ", -" SIM_OPT_STATEPATH_SHORT ") directory contains no .json files");
  }

  std::vector<named_simulation> simulations;
  simulations.reserve(state_files.size());

  // parse all state files in provided path and register them for simulation
  for (auto const &state_file_path : state_files) {
    std::string const path_str = state_file_path.string();

    std::variant<simulation::state, errors_t> parse_res = simulation::parse_state(
      util::extract_txt_file_contents(path_str, false),
      s_cfg.state_path
    );

    if (std::holds_alternative<errors_t>(parse_res)) {
      errors_t const &errors = std::get<errors_t>(parse_res);
      for (auto const &err : errors)
        print_err(err.c_str());
      die("%zu state errors", errors.size());
    }

    std::string name = state_file_path.filename().string();
    simulation::state &state = std::get<simulation::state>(parse_res);
    simulations.emplace_back(std::move(name), state);
  }

  auto const simulation_task = [](named_simulation &sim) {
    ++s_num_simulations_in_progress;

    try {
      using logger::event_type;

      logger::log(event_type::SIM_START, sim.name.c_str());

      auto const result = simulation::run(
        sim.state,
        sim.name,
        s_cfg.generation_limit,
        s_cfg.save_points,
        s_cfg.save_interval,
        s_cfg.img_fmt,
        s_cfg.save_path,
        s_cfg.save_final_state,
        s_cfg.log_save_points,
        s_cfg.save_image_only
      );

      logger::log(
        event_type::SIM_END,
        "%s %s",
        sim.name.c_str(),
        (result.code == simulation::run_result::REACHED_GENERATION_LIMIT
          ? "reached generation limit"
          : "hit edge")
      );

      ++s_num_simulations_done;

    } catch (...) {
      ++s_num_simulations_failed;
    }

    --s_num_simulations_in_progress;
    delete[] sim.state.grid;
  };

  BS::thread_pool_light t_pool(num_threads);

#if _WIN32
  // I would const this, but native_handle() is not a const member function for some reason
  std::thread *threads = t_pool.get_threads_uniqueptr().get();

  for (usize i = 0; i < t_pool.get_thread_count(); ++i) {
    SetPriorityClass(threads[i].native_handle(), HIGH_PRIORITY_CLASS);
  }
#endif

  for (auto &sim : simulations) {
    t_pool.push_task(simulation_task, std::ref(sim));
  }

  // sleep for a bit so we are not dividing by tiny time elapsed values,
  // which results in Infinity values
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  term::cursor::hide();
  std::atexit(term::cursor::show);

  ui_loop(simulations);

  return static_cast<int>(s_num_simulations_failed.load());
}

std::string usage_msg()
{
  std::ostringstream msg;

  msg <<
    "\n"
    "USAGE: \n"
    "  simulate_many <num_threads> <log_path> [options] \n"
    "\n"
  ;

  simulation::env_options_description(
    "path to directory containing initial state .json files"
  ).print(msg, 6);

  msg <<
    "\n"
    "*** = required \n"
    "\n"
  ;

  return msg.str();
}

void ui_loop(std::vector<named_simulation> const &simulations)
{
  using namespace term::color;

  std::string const horizontal_rule(30, '-');
  usize const digits_in_num_simulations = util::count_digits(simulations.size());

  time_point_t const start_time = util::current_time();

  for (;;) {
    time_point_t const time_now = util::current_time();

    u64
      gens_completed = 0,
      nanos_spent_iterating = 0,
      nanos_spent_saving = 0;
    for (auto const &sim : simulations) {
      gens_completed += sim.state.generation;
      auto const time_breakdown = simulation::query_activity_time_breakdown(sim.state, time_now);
      nanos_spent_iterating += time_breakdown.nanos_spent_iterating;
      nanos_spent_saving += time_breakdown.nanos_spent_saving;
    }

    u64 const
      num_sims_completed = s_num_simulations_done.load(),
      num_sims_failed = s_num_simulations_failed.load(),
      num_sims_in_progress = s_num_simulations_in_progress.load(),
      total_nanos_elapsed = util::nanos_between(start_time, time_now).count();
    f64 const
      total_secs_elapsed = total_nanos_elapsed / 1'000'000'000.0,
      secs_elapsed_iterating = nanos_spent_iterating / 1'000'000'000.0,
      secs_elapsed_saving = nanos_spent_saving / 1'000'000'000.0,
      mega_gens_completed = gens_completed / 1'000'000.0,
      mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed_iterating, 0.0 + DBL_EPSILON);

    // line
    puts(horizontal_rule.c_str());

    // line
    {
      printf("Mgens/sec : ");
      printf(fore::WHITE | back::MAGENTA, "%.2lf", mega_gens_per_sec);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    {
      printf("Completed : ");
      printf(fore::LIGHT_GREEN | back::BLACK, "%zu", num_sims_completed);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    {
      printf("Active    : ");
      printf(fore::LIGHT_YELLOW | back::BLACK, "%zu", num_sims_in_progress);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    {
      printf("Failed    : ");
      printf(fore::LIGHT_RED | back::BLACK, "%zu", num_sims_failed);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    {
      printf("Elapsed   : %s", timespan_to_string(timespan_calculate(static_cast<u64>(total_secs_elapsed))).c_str());
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    {
      printf("I/S Ratio : %.1lf / %.1lf",
        ( nanos_spent_iterating / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0,
        ( nanos_spent_saving    / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line
    puts(horizontal_rule.c_str());

    if (num_sims_completed == simulations.size()) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    term::cursor::move_up(8);
  }
}
