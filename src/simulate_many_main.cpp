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
#include <boost/multiprecision/cpp_int.hpp>
#include "lib/BS_thread_pool.hpp"
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
#include "platform.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::time_point_t;
using util::errors_t;
using util::print_err;
using util::make_str;
using util::die;

static po::simulate_many_options s_options{};
static std::mutex stdout_mutex{};
static std::mutex stderr_mutex{};

i32 main(i32 const argc, char const *const *const argv)
try
{
  using namespace term::color;

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

  if (s_options.log_file_path != "") {
    logger::set_out_file_path(s_options.log_file_path);
    logger::set_autoflush(true);
  }

  std::vector<fs::path> const state_files = fregex::find(
    s_options.state_dir_path.c_str(),
    ".*\\.json",
    fregex::entry_type::regular_file);

  assert(!state_files.empty());
  assert(state_files.size() <= u64(std::numeric_limits<std::ptrdiff_t>::max()));

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

  // For processing multiple simulations at once
  BS::thread_pool t_pool(s_options.num_threads);

#if ON_WINDOWS
  std::thread *threads = t_pool.get_threads().get();
  for (u32 i = 0; i < t_pool.get_thread_count(); ++i) {
    SetPriorityClass(threads[i].native_handle(), HIGH_PRIORITY_CLASS);
  }
#endif

  time_point_t const start_time = util::current_time();

  // parse state files into initial simulation states and add them to the simulation_queue
  std::thread producer_thread([&]() {
    assert(state_files.size() - 1 < static_cast<u64>(std::numeric_limits<i64>::max()));

    for (i64 i = state_files.size() - 1; i >= 0; --i) {
      sem_empty.acquire();
      {
        std::scoped_lock sim_q_lock(simulation_queue_mutex);
        errors_t errors{};
        std::string const path_str = state_files[i].generic_string();

        auto state = simulation::parse_state(
          util::extract_txt_file_contents(path_str.c_str(), false),
          fs::path(s_options.state_dir_path),
          errors);

        if (!errors.empty()) {
          if (s_options.any_logging_enabled()) {
            std::string const err = make_str("failed to parse %s: %s", path_str.c_str(), util::stringify_errors(errors).c_str());
            if (s_options.log_file_path != "")
              logger::log(logger::event_type::ERR, "%s", err.c_str());
            if (s_options.log_to_stdout) {
              std::scoped_lock stdout_lock(stdout_mutex);
              std::printf("%s\n", err.c_str());
            }
          }
        } else {
          simulation_queue.emplace_back(simulation::extract_name_from_json_state_path(path_str), state);
        }
      }
      sem_full.release();
    }
  });

  auto const simulation_task = [&](named_simulation &sim) {
    time_point_t const start_time = util::current_time();

    if (s_options.log_to_stdout) {
      std::scoped_lock stdout_lock(stdout_mutex);
      auto const time = std::time(NULL);
      char *const time_cstr = ctime(&time);
      time_cstr[std::strlen(time_cstr) - 1] = '\0'; // remove trailing \n
      std::printf("[%s] %s\n", time_cstr, sim.name.c_str());
    }

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
        s_options.sim.create_logs,
        s_options.sim.save_image_only
      );

      [[maybe_unused]] time_point_t const end_time = util::current_time();
      auto const time_breakdown = simulation::query_activity_time_breakdown(sim.state);

      {
        std::scoped_lock compl_sims_lock(completed_simulations_mutex);
        completed_simulations.emplace_back(sim.state.generation, time_breakdown, result);
      }

      if (s_options.log_to_stdout) {
        auto const time = std::time(NULL);
        char *const time_cstr = ctime(&time);
        time_cstr[std::strlen(time_cstr) - 1] = '\0'; // remove trailing \n
        u64 const
          which_sim_num = num_simulations_processed.load() + 1,
          gens_completed = sim.state.generations_completed();
        f64 const
          percent_to_gen_limit = (gens_completed / static_cast<f64>(s_options.sim.generation_limit)) * 100.0,
          mega_gens_completed = gens_completed / 1'000'000.0,
          mega_gens_per_sec = mega_gens_completed / (time_breakdown.nanos_spent_iterating / 1'000'000'000.0),
          seconds_elapsed = util::nanos_between(start_time, end_time) / 1'000'000'000.0;
        // std::string const time_elapsed = util::time_span(static_cast<u64>(seconds_elapsed)).to_string();
        char time_elapsed[64];
        util::time_span(static_cast<u64>(seconds_elapsed)).stringify(time_elapsed, util::lengthof(time_elapsed));

        std::scoped_lock stdout_lock(stdout_mutex);

        term::color::printf(fore::GREEN, "[%s] (%zu/%zu) %s | %zu (%.1lf %%) | %s | %.2lf Mgens/sec\n",
          time_cstr, which_sim_num, state_files.size(), sim.name.c_str(),
          gens_completed, percent_to_gen_limit, time_elapsed, mega_gens_per_sec);
      }
    } catch (std::exception const &except) {
      if (s_options.any_logging_enabled()) {
        std::string const err = make_str("%s failed: %s", except.what());
        if (s_options.log_file_path != "")
          logger::log(logger::event_type::ERR, "%s", err.c_str());
        if (s_options.log_to_stdout)
          print_err("%s", err.c_str());
      }
    } catch (...) {
      if (s_options.any_logging_enabled()) {
        std::string const err = make_str("%s failed, unknown cause", sim.name.c_str());
        if (s_options.log_file_path != "")
          logger::log(logger::event_type::ERR, "%s", err.c_str());
        if (s_options.log_to_stdout)
          print_err("%s", err.c_str());
      }
    }

    delete[] sim.state.grid;
    ++num_simulations_processed;
  };

  // submit parsed simulation states to the thread pool for simulation as they arrive
  std::thread consumer_thread([&]() {
    for (u64 i = 0; i < state_files.size(); ++i) {
      sem_full.acquire();
      {
        std::scoped_lock sim_q_lock(simulation_queue_mutex);
        t_pool.push_task(simulation_task, std::move(simulation_queue.back()));
        simulation_queue.pop_back();
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
    total_secs_elapsed = total_nanos_elapsed / 1'000'000'000.0,
    secs_spent_iterating = nanos_spent_iterating / 1'000'000'000.0,
    mega_gens_completed = gens_completed / 1'000'000.0,
    mega_gens_per_sec = mega_gens_completed / std::max(secs_spent_iterating, 0.0 + DBL_EPSILON),
    percent_iteration = ( nanos_spent_iterating / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0,
    percent_saving    = ( nanos_spent_saving    / (static_cast<f64>(nanos_spent_iterating + nanos_spent_saving)) ) * 100.0;

  std::printf("------------------------------\n");
  std::printf("Mgens/sec : %.1lf\n", mega_gens_per_sec);
  std::printf("I/S Ratio : %.1lf / %.1lf\n",
      std::isnan(percent_iteration) ? 0.0 : percent_iteration,
      std::isnan(percent_saving)    ? 0.0 : percent_saving);
  std::printf("Elapsed   : %s\n", util::time_span(static_cast<u64>(total_secs_elapsed)).to_string().c_str());
  std::printf("------------------------------\n");

  return 0;
}
catch (std::exception const &except)
{
  std::scoped_lock stderr_lock(stderr_mutex);
  std::cerr << except.what() << '\n';
  return 1;
}
catch (std::string const &except)
{
  std::scoped_lock stderr_lock(stderr_mutex);
  std::cerr << except << '\n';
  return 1;
}
catch (char const *const except)
{
  std::scoped_lock stderr_lock(stderr_mutex);
  std::cerr << except << '\n';
  return 1;
}
catch (...)
{
  std::scoped_lock stderr_lock(stderr_mutex);
  std::cerr << "unknown error - catch (...)" << '\n';
  return 1;
}
