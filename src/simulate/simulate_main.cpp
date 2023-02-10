#pragma warning(push, 0)
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif
#include "../json.hpp"
#pragma warning(pop)

#include "../primitives.hpp"
#include "../term.hpp"
#include "../timespan.hpp"
#include "../util.hpp"
#include "exit.hpp"
#include "prgopts.hpp"
#include "simulation.hpp"

namespace bpo = boost::program_options;
using namespace term::color;
using time_point = std::chrono::steady_clock::time_point;
using util::strvec_t;
using json_t = nlohmann::json;

time_point current_time()
{
  return std::chrono::high_resolution_clock::now();
}

auto nanos_between(time_point const a, time_point const b)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a);
}

template <typename Ty>
Ty get_required_arg(char const *const argname, bpo::variables_map const &args)
{
  if (args.count(argname) == 0) {
    std::cerr << "missing required argument --" << argname;
    die(exit_code::MISSING_REQUIRED_ARGUMENT);
  } else {
    return args.at(argname).as<Ty>();
  }
}

template <typename Ty>
std::optional<Ty> get_optional_arg(char const *const argname, bpo::variables_map const &args)
{
  if (args.count(argname) == 0) {
    return std::nullopt;
  } else {
    return args.at(argname).as<Ty>();
  }
}

b8 get_flag_arg(char const *const argname, bpo::variables_map const &args)
{
  return args.count(argname) > 0;
}

void update_ui(
  std::string const sim_name,
  u64 const generation_target,
  simulation::state const &state)
{
  static usize ticks = 0;
  time_point const start_time = current_time();

  for (;;) {
    time_point const time_now = current_time();
    u64 const nanos_elapsed = nanos_between(start_time, time_now).count();
    f64 const secs_elapsed = nanos_elapsed / 1'000'000'000.0;
    u64 const gens_completed = state.generation;
    f64 const mega_gens_completed = gens_completed / 1'000'000.0;
    f64 const mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed, 0.0 + DBL_EPSILON);
    f64 const percent_completion = ((gens_completed / static_cast<f64>(generation_target)) * 100.0);
    u64 const gens_remaining = generation_target - gens_completed;
    b8 const is_simulation_done = !state.can_step_forward(generation_target);

    // line 1, print at beginning and end
    if (ticks == 0 || is_simulation_done) {
      printf(fore::CYAN | back::BLACK, "%s", sim_name.c_str());
      printf(fore::DEFAULT | back::BLACK, " - ");

      if (gens_remaining > 0) {
        printf(fore::YELLOW | back::BLACK, "in progress");
      } else {
        printf(fore::DEFAULT | back::BLACK, "finished, %s", [&state]() {
          switch (state.last_step_res) {
            case simulation::step_result::NIL:
              return "internal error (NIL step result)";
            case simulation::step_result::SUCCESS:
              return "generation target reached";
            case simulation::step_result::FAILED_AT_BOUNDARY:
              return "tried to step off grid";
            default:
              return "(nullptr)";
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
        gens_completed,
        percent_completion);
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

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    puts(prgopts::usage_msg());
    die(exit_code::MISSING_REQUIRED_ARGUMENT);
  }

  int const err_style = fore::RED | back::BLACK;

  // parse command line args
  bpo::variables_map args;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, prgopts::options_descrip()), args);
  } catch (std::exception const &err) {
    printf(err_style, "fatal: %s\n", err.what());
    die(exit_code::INVALID_OPTION_SYNTAX);
  }
  bpo::notify(args);

  std::string const sim_name = get_required_arg<std::string>("name", args);
  std::string const sim_file_pathname = get_required_arg<std::string>("cfg", args);
  u64 const generation_target = get_required_arg<u64>("maxgen", args);

  pgm8::format const img_fmt = [&args]() {
    if (args.count("imgfmt") == 0) {
      return pgm8::format::RAW;
    } else {
      std::string const fmt = args.at("imgfmt").as<std::string>();
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
    printf(err_style, "fatal: invalid --imgfmt, must be rawPGM|plainPGM\n");
    die(exit_code::BAD_ARGUMENT_VALUE);
  }

  simulation::state state;
  strvec_t errors{};

  state = simulation::parse_state(
    util::extract_txt_file_contents(sim_file_pathname.c_str()),
    std::filesystem::current_path(),
    errors
  );

  if (!errors.empty()) {
    printf(fore::RED | back::BLACK, "fatal: malformed configuration (%zu errors)\n", errors.size());

    for (auto const &err : errors)
      printf(fore::RED | back::BLACK, "  %s\n", err.c_str());

    die(exit_code::BAD_CONFIG);
  }

  auto const save_points = [&args]() -> std::vector<u64> {
    auto const save_points_str = get_optional_arg<std::string>("savepoints", args);
    if (!save_points_str.has_value()) {
      return {};
    }
    try {
      return util::parse_json_array_u64(save_points_str.value().c_str());
    } catch (json_t::parse_error const &) {
      printf(err_style, "fatal: unable to parse --savepoints, not a JSON array\n");
      die(exit_code::BAD_ARGUMENT_VALUE);
    } catch (std::runtime_error const &except) {
      printf(err_style, "fatal: unable to parse --savepoints, %s\n", except.what());
      die(exit_code::BAD_ARGUMENT_VALUE);
    }
  }();

  u64 const save_interval = get_optional_arg<u64>("saveinterval", args).value_or(0ui64);

  std::thread sim_thread(
    simulation::run,
    std::ref(state),
    sim_name,
    generation_target,
    save_points,
    save_interval,
    img_fmt,
    std::filesystem::current_path(),
    get_flag_arg("savefinalstate", args)
  );

#ifdef _WIN32
  SetPriorityClass(sim_thread.native_handle(), HIGH_PRIORITY_CLASS);
#endif

  std::thread ui_update_thread(
    update_ui,
    sim_name,
    generation_target,
    std::ref(state)
  );

  term::cursor::hide();
  ui_update_thread.join();
  sim_thread.join();
  term::cursor::show();

  die(exit_code::SUCCESS);
}
