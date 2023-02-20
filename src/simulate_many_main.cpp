#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>
#include "lib/BS_thread_pool_light.hpp"
#include "lib/json.hpp"
#include "lib/term.hpp"
#include "lib/regexglob.hpp"
#include "lib/logger.hpp"

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include "primitives.hpp"
#include "timespan.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::time_point_t;
using util::errors_t;
using util::print_err;
using util::die;

#define OPT_THREADS_FULL "threads"
#define OPT_THREADS_SHORT "t"

static simulation::env_config s_cfg{};

// bpo::options_description options_descrip();
std::string usage_msg();

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    std::cout << usage_msg();
    std::exit(1);
  }

  u32 num_threads = 0;
  try {
    num_threads = std::stoul(argv[1]);
  } catch (...) {
    die("unable to parse <num_threads>");
  }

  // {
  //   bpo::variables_map var_map;
  //   try {
  //     bpo::store(bpo::parse_command_line(argc, argv, options_descrip()), var_map);
  //   } catch (std::exception const &err) {
  //     die(err.what());
  //   }
  //   bpo::notify(var_map);

  //   if (var_map.count(OPT_THREADS_FULL) == 0) {
  //   } else {
  //     try {
  //       num_threads = var_map.at(OPT_THREADS_FULL).as<usize>();
  //     } catch (...) {
  //       die("(--" OPT_THREADS_FULL ", -" OPT_THREADS_SHORT ") unable to parse value");
  //     }
  //   }
  // }

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
      die("(--" SIM_OPT_STATEPATH_SHORT ", -" SIM_OPT_STATEPATH_SHORT ") path is not a directory");
    }
  }

  logger::set_out_pathname("./.log");
  logger::set_autoflush(true);

  std::vector<fs::path> const state_files = regexglob::fmatch(
    s_cfg.state_path.string().c_str(),
    ".*\\.json"
  );

  if (state_files.empty()) {
    die("supplied --states directory has no JSON files");
  }

  struct named_simulation
  {
    std::string name;
    simulation::state state;
  };

  std::vector<named_simulation> simulations;
  simulations.reserve(state_files.size());

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
    using logger::event_type;

    logger::log(event_type::INF, "%s started", sim.name.c_str());

    auto const result = simulation::run(
      sim.state,
      sim.name,
      s_cfg.generation_limit,
      s_cfg.save_points,
      s_cfg.save_interval,
      s_cfg.img_fmt,
      s_cfg.save_path,
      s_cfg.save_final_state
    );

    logger::log(
      event_type::INF,
      "%s finished, %s",
      sim.name.c_str(),
      (result.code == simulation::run_result::REACHED_GENERATION_LIMIT
        ? "reached generation limit"
        : "hit edge")
    );
  };

  BS::thread_pool_light t_pool(num_threads);

  for (auto &sim : simulations) {
    // simulation_task(sim);
    t_pool.push_task(simulation_task, sim);
  }

  t_pool.wait_for_tasks();

  return 0;
}

std::string usage_msg()
{
  std::ostringstream msg;

  msg <<
    "\n"
    "USAGE: \n"
    "  simulate_many <num_threads> [options] \n"
    "\n"
  ;

  // options_descrip().print(msg, 6);
  // msg << '\n';

  simulation::env_options_descrip(
    "path to directory containing initial state .json files"
  ).print(msg, 6);

  msg <<
    "\n"
    "*** = required \n"
    "\n"
  ;

  return msg.str();
}

// bpo::options_description options_descrip()
// {
//   bpo::options_description desc("GENERAL OPTIONS");

//   desc.add_options()
//     (OPT_THREADS_FULL "," OPT_THREADS_SHORT, bpo::value<usize>(), "size of thread pool, > 0 uint64")
//   ;

//   return desc;
// }
