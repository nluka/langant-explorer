#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>
#include <tuple>
#include <semaphore>
#include <mutex>

#include <boost/program_options.hpp>

#include "program_options.hpp"
#include "util.hpp"
#include "term.hpp"
#include "logger.hpp"
#include "fregex.hpp"
#include "BS_thread_pool.hpp"
#include "simulation.hpp"

#ifdef ON_WINDOWS
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::time_point_t;
using util::errors_t;
using util::print_err;
using util::make_str;
using util::die;

static po::simulate_many_options s_options{};

i32 main(i32 const argc, char const *const *const argv)
try
{
  using namespace term;

  struct named_simulation
  {
    std::string name;
    simulation::state state;
  };

  typedef std::tuple<
    u64, // generation
    simulation::activity_time_breakdown,
    simulation::run_result
  > completed_simulation_t;

  if (argc < 2) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "Usage:\n"
      "  simulate_many [options]\n"
      "\n";
    po::simulate_many_options_description().print(usage_msg, 6);
    usage_msg <<
      "\n"
      "Additional Notes:\n"
      "  - Each queue slot requires " << sizeof(named_simulation) << " bytes for the duration of the program\n"
      "  - Each in-flight simulation (# determined by thread pool size) requires " << sizeof(named_simulation) << " bytes of storage\n"
      "     plus whatever the grid (image) requires, where each cell (pixel) occupies 1 byte\n"
      "  - Each simulation requires " << sizeof(completed_simulation_t) << " bytes of storage for the duration of the program\n"
      "\n";
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

  logger::set_out_file_path(s_options.log_file_path);
  logger::set_stdout_logging(s_options.log_to_stdout);
  logger::set_autoflush(true);
  logger::set_delim("\n");

  std::vector<fs::path> const state_files = fregex::find(
    s_options.state_dir_path.c_str(),
    ".*\\.json",
    fregex::entry_type::regular_file);

  if (state_files.empty())
    die("no state files found in '%s'", s_options.state_dir_path.c_str());

  {
    auto const ptrdiff_max = std::numeric_limits<std::ptrdiff_t>::max();

    if (state_files.size() > u64(ptrdiff_max))
      die("too many state files in '%s', can only handle up to %ld",
        s_options.state_dir_path.c_str(), ptrdiff_max);
  }

  // A "queue", but using a vector because we don't care about the order of execution
  std::vector<named_simulation> simulation_queue(s_options.queue_size);
  std::mutex simulation_queue_mutex{};

  std::vector<completed_simulation_t> completed_simulations{};
  std::mutex completed_simulations_mutex{};

  std::atomic<u64> num_simulations_processed(0);

  std::counting_semaphore sem_full{0};
  std::counting_semaphore sem_empty{static_cast<std::ptrdiff_t>(state_files.size())};

  simulation_queue.clear();
  completed_simulations.reserve(state_files.size());

  // for processing multiple simulations at once
  BS::thread_pool t_pool(s_options.num_threads);

  std::thread *threads = t_pool.get_threads().get();
  for (u32 i = 0; i < t_pool.get_thread_count(); ++i)
    util::set_thread_priority_high(threads[i]);

  time_point_t const start_time = util::current_time();

  // parse state files into initial simulation states and add them to the simulation_queue
  std::thread producer_thread([&]() {

    for (i64 i = i64(state_files.size()) - 1; i >= 0; --i) {
      sem_empty.acquire();
      {
        std::scoped_lock sim_q_lock(simulation_queue_mutex);
        errors_t errors{};
        std::string const path_str = state_files[u64(i)].generic_string();

        simulation::state state;
        try {
          state = simulation::parse_state(
            util::extract_txt_file_contents(path_str.c_str(), false),
            fs::path(s_options.state_dir_path),
            errors);

          if (!errors.empty()) {
            if (s_options.any_logging_enabled()) {
              std::string const err = make_str("failed to parse %s: %s", path_str.c_str(), util::stringify_errors(errors).c_str());
              logger::log(logger::event_type::ERROR, "%s", err.c_str());
            }
          } else {
            simulation_queue.emplace_back(simulation::extract_name_from_json_state_path(path_str), state);
          }
        } catch (std::exception const &except) {
          logger::log(logger::event_type::ERROR, "%s", except.what());
        } catch (...) {
          logger::log(logger::event_type::ERROR, "catch (...) in producer_thread");
        }
      }
      sem_full.release();
    }
  });

  auto const simulation_task = [&](named_simulation &sim, u64 const total) {
    try {
      auto const result = simulation::run(
        sim.state,
        sim.name,
        s_options.sim.generation_limit,
        s_options.sim.save_points,
        s_options.sim.save_interval,
        s_options.sim.image_format,
        s_options.sim.save_path,
        s_options.sim.save_final_state,
        s_options.any_logging_enabled(),
        s_options.sim.save_image_only,
        &num_simulations_processed,
        total);

      [[maybe_unused]] time_point_t const end_time = util::current_time();
      auto const time_breakdown = sim.state.query_activity_time_breakdown();

      {
        std::scoped_lock compl_sims_lock(completed_simulations_mutex);
        completed_simulations.emplace_back(sim.state.generation, time_breakdown, result);
      }
    } catch (std::exception const &except) {
      if (s_options.any_logging_enabled()) {
        std::string const err = make_str("%s failed: %s", except.what());
        logger::log(logger::event_type::ERROR, "%s", err.c_str());
      }
    } catch (...) {
      if (s_options.any_logging_enabled()) {
        std::string const err = make_str("%s failed, unknown cause - catch (...)", sim.name.c_str());
        logger::log(logger::event_type::ERROR, "%s", err.c_str());
      }
    }

    delete[] sim.state.grid;
    ++num_simulations_processed;
  };

  // submit parsed simulation states to the thread pool for simulation as they arrive
  std::thread consumer_thread([&]() {
    for (u64 i = 0; i < state_files.size(); ++i) {
      sem_full.acquire();
      try {
        std::scoped_lock sim_q_lock(simulation_queue_mutex);
        t_pool.push_task(simulation_task, std::move(simulation_queue.back()), state_files.size());
        simulation_queue.pop_back();
      } catch (std::exception const &except) {
        logger::log(logger::event_type::ERROR, "%s", except.what());
      } catch (...) {
        logger::log(logger::event_type::ERROR, "catch (...) in consumer_thread");
      }
      sem_empty.release();
    }
  });

  producer_thread.join();
  consumer_thread.join();
  t_pool.wait_for_tasks();

  time_point_t const end_time = util::current_time();

  u64
    gens_completed = 0,
    nanos_spent_iterating = 0,
    nanos_spent_saving = 0;
  for (auto const &sim : completed_simulations) {
    gens_completed += std::get<0>(sim);
    nanos_spent_iterating += std::get<1>(sim).nanos_spent_iterating;
    nanos_spent_saving += std::get<1>(sim).nanos_spent_saving;
  }

  u64 const
    total_nanos_elapsed = util::nanos_between(start_time, end_time);
  f64 const
    f64_epsilon = std::numeric_limits<f64>::epsilon(),
    total_secs_elapsed = f64(total_nanos_elapsed) / 1'000'000'000.0,
    secs_spent_iterating = f64(nanos_spent_iterating) / 1'000'000'000.0,
    mega_gens_completed = f64(gens_completed) / 1'000'000.0,
    mega_gens_per_sec = mega_gens_completed / std::max(secs_spent_iterating, 0.0 + f64_epsilon),
    percent_iteration = ( f64(nanos_spent_iterating) / f64(nanos_spent_iterating + nanos_spent_saving) ) * 100.0,
    percent_saving    = ( f64(nanos_spent_saving   ) / f64(nanos_spent_iterating + nanos_spent_saving) ) * 100.0;

  if (s_options.any_logging_enabled()) {
    // print separator between logs and statistics
    std::printf("-----------------------------------\n");
  }
  std::printf("Avg Mgens/sec : %.2lf\n", mega_gens_per_sec);
  std::printf("Avg I/S Ratio : %.2lf / %.2lf\n",
      std::isnan(percent_iteration) ? 0.0 : percent_iteration,
      std::isnan(percent_saving)    ? 0.0 : percent_saving);

  {
    char time_elapsed[64];
    util::time_span(static_cast<u64>(total_secs_elapsed)).stringify(time_elapsed, util::lengthof(time_elapsed));
    std::printf("Time Elapsed  : %s\n", time_elapsed);
  }

  return 0;
}
catch (std::exception const &except)
{
  die("%s", except.what());
}
catch (...)
{
  die("unknown error - catch (...)");
}
