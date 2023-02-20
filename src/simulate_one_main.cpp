#include <algorithm>
#include <cstdarg>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <variant>

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include <boost/program_options.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "primitives.hpp"
#include "timespan.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

static simulation::env_config s_cfg{};
static simulation::state s_sim_state{};

std::string usage_msg();
void update_ui(std::string const &name);

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    std::cout << usage_msg();
    std::exit(1);
  }

  {
    std::variant<
      simulation::env_config,
      errors_t
    > extract_res = simulation::extract_env_config(argc, argv, "path to initial state .json file");

    if (std::holds_alternative<errors_t>(extract_res)) {
      errors_t const &errors = std::get<errors_t>(extract_res);
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      die("%zu configuration errors", errors.size());
    }

    s_cfg = std::move(std::get<simulation::env_config>(extract_res));

    // some additional validation specific to this program
    if (!fs::is_directory(s_cfg.state_path)) {
      die("(--" SIM_OPT_STATEPATH_SHORT ", -" SIM_OPT_STATEPATH_SHORT ") path is not a file");
    }
  }

  std::variant<
    simulation::state,
    errors_t
  > parse_res = simulation::parse_state(
    util::extract_txt_file_contents(s_cfg.state_path.string(), false),
    std::filesystem::current_path()
  );

  if (std::holds_alternative<errors_t>(parse_res)) {
    errors_t const &errors = std::get<errors_t>(parse_res);
    for (auto const &err : errors)
      print_err(err.c_str());
    die("%zu state errors", errors.size());
  }

  std::string const name = s_cfg.state_path.filename().string();
  s_sim_state = std::get<simulation::state>(parse_res);

  std::thread sim_thread([&name]() {
    simulation::run(
      s_sim_state,
      name,
      s_cfg.generation_limit,
      s_cfg.save_points,
      s_cfg.save_interval,
      s_cfg.img_fmt,
      s_cfg.save_path,
      s_cfg.save_final_state
    );
  });

#ifdef _WIN32
  SetPriorityClass(sim_thread.native_handle(), HIGH_PRIORITY_CLASS);
#endif

  std::thread ui_update_thread(update_ui, name); // function runs until simulation is finished

  term::cursor::hide();
  std::atexit(term::cursor::show);
  ui_update_thread.join();
  sim_thread.join();

  return 0;
}

std::string usage_msg()
{
  std::ostringstream msg;

  msg <<
    "\n"
    "USAGE: \n"
    "  simulate_one [options] \n"
    "\n"
  ;

  simulation::env_options_descrip(
    "path to initial state .json file"
  ).print(msg, 6);

  msg <<
    "\n"
    "*** = required \n"
    "\n"
  ;

  return msg.str();
}

void update_ui(std::string const &name)
{
  using namespace term::color;
  using util::time_point_t;

  time_point_t const start_time = util::current_time();

  for (;;) {
    time_point_t const time_now = util::current_time();
    u64 const
      nanos_elapsed = util::nanos_between(start_time, time_now).count(),
      gens_completed = s_sim_state.generation,
      gens_remaining = s_cfg.generation_limit - gens_completed;
    f64 const
      secs_elapsed = nanos_elapsed / 1'000'000'000.0,
      mega_gens_completed = gens_completed / 1'000'000.0,
      mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed, 0.0 + DBL_EPSILON),
      percent_completion = ((gens_completed / static_cast<f64>(s_cfg.generation_limit)) * 100.0);
    b8 const is_simulation_done = !s_sim_state.can_step_forward(s_cfg.generation_limit);

    // line 1
    {
      printf(fore::CYAN | back::BLACK, "%s", name.c_str());
      printf(fore::DEFAULT | back::BLACK, " - ");

      if (!is_simulation_done) {
        printf(fore::YELLOW | back::BLACK, "in progress");
      } else {
        printf(fore::DEFAULT | back::BLACK, "finished, %s", []() {
          switch (s_sim_state.last_step_res) {
            case simulation::step_result::NIL: return "internal error (NIL step result)";
            case simulation::step_result::SUCCESS: return "generation limit reached";
            case simulation::step_result::HIT_EDGE: return "hit edge";
            default: return "internal error (nullptr)";
          }
        }());
      }

      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line 2
    {
      printf(
        fore::DEFAULT | back::BLACK,
        "generation %llu, %.1lf%%",
        gens_completed,
        percent_completion);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line 3
    {
      printf(fore::DEFAULT | back::BLACK, "%.2lf Mgens/sec", mega_gens_per_sec);
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    // line 4
    {
      printf(
        fore::DEFAULT | back::BLACK,
        "%s elapsed",
        timespan_to_string(timespan_calculate(static_cast<usize>(secs_elapsed))).c_str());
      term::clear_to_end_of_line();
      putc('\n', stdout);
    }

    if (is_simulation_done)
      return;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    term::cursor::move_up(4);
  }
}
