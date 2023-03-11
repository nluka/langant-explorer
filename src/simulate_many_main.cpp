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

std::string usage_msg();
void update_user_interface(usize num_simulations);

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

  struct named_simulation
  {
    std::string name;
    simulation::state state;
  };

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
    t_pool.push_task(simulation_task, sim);
  }

  term::cursor::hide();
  std::atexit(term::cursor::show);

  for (;;) {
    usize const num_sims_completed = s_num_simulations_done.load();
    usize const num_sims_failed = s_num_simulations_failed.load();
    usize const num_sims_in_progress = s_num_simulations_in_progress.load();

    printf("%zu / %zu simulations completed, %zu in progress, %zu failed\n",
      num_sims_completed, simulations.size(), num_sims_in_progress, num_sims_failed);

    if (num_sims_completed == simulations.size()) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    term::cursor::move_up(1);
  }

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
