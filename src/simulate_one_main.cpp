#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include <boost/program_options.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "prgopt.hpp"
#include "primitives.hpp"
#include "timespan.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;
using namespace term::color;
using time_point = std::chrono::steady_clock::time_point;
using util::strvec_t;
using json_t = nlohmann::json;
using prgopt::get_required_arg;
using prgopt::get_optional_arg;
using prgopt::get_flag_arg;
using util::die;
using util::time_point_t;

enum class exit_code : int
{
  GENERIC_ERROR = -1,
  SUCCESS = 0,
  INVALID_ARGUMENT_SYNTAX,
  MISSING_REQUIRED_ARGUMENT,
  BAD_ARGUMENT_VALUE,
  FILE_OPEN_FAILED,
  BAD_STATE,
};

char const *usage_msg()
{
  return
    "USAGE \n"
    "  simulate_one --cfg <path> --maxgen <int> [--name <name>] \n"
    "               [--imgfmt <fmt>] [--savefinalstate] [--save_points <json_array>] \n"
    "               [--save_interval <int>] [--savedir <path>] \n"
    "REQUIRED \n"
    "  state ........... path to initial state JSON file \n"
    "  maxgen .......... generation limit, uint64, > 0 \n"
    "OPTIONAL \n"
    "  name ............ string, name of simuation, defaults to --state file name w/o extension \n"
    "  imgfmt .......... string, rawPGM|plainPGM, default=rawPGM \n"
    "  savefinalstate .. flag, guarantees final state is saved, off by default \n"
    "  savepoints ...... JSON uint64 array, specific generations to save \n"
    "  saveinterval .... > 0 uint64 \n"
    "  savedir ......... path, where save files (.json, .pgm) are emitted, default=cwd \n"
  ;
}

bpo::options_description my_boost_options()
{
  bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()
    ("name", bpo::value<std::string>(), empty_desc)
    ("cfg", bpo::value<std::string>(), empty_desc)
    ("maxgen", bpo::value<u64>(), empty_desc)
    ("imgfmt", bpo::value<std::string>(), empty_desc)
    ("savefinalstate", /* flag */ empty_desc)
    ("savepoints", bpo::value<std::string>(), empty_desc)
    ("saveinterval", bpo::value<u64>(), empty_desc)
    ("savedir", bpo::value<std::string>(), empty_desc)
  ;

  return desc;
}

void update_ui(
  std::string const sim_name,
  u64 const generation_target,
  simulation::state const &state)
{
  static usize ticks = 0;
  time_point_t const start_time = util::current_time();

  for (;;) {
    time_point const time_now = util::current_time();
    u64 const
      nanos_elapsed = util::nanos_between(start_time, time_now).count(),
      gens_completed = state.generation,
      gens_remaining = generation_target - gens_completed;
    f64 const
      secs_elapsed = nanos_elapsed / 1'000'000'000.0,
      mega_gens_completed = gens_completed / 1'000'000.0,
      mega_gens_per_sec = mega_gens_completed / std::max(secs_elapsed, 0.0 + DBL_EPSILON),
      percent_completion = ((gens_completed / static_cast<f64>(generation_target)) * 100.0);
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
              return "generation limit reached";
            case simulation::step_result::HIT_EDGE:
              return "tried to step off grid";
            default:
              return "internal error (nullptr)";
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
    puts(usage_msg());
    die(exit_code::MISSING_REQUIRED_ARGUMENT);
  }

  int const
    err_style = fore::RED | back::BLACK,
    missing_arg_ec = static_cast<int>(exit_code::MISSING_REQUIRED_ARGUMENT);

  // parse command line args
  bpo::variables_map args;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, my_boost_options()), args);
  } catch (std::exception const &err) {
    printf(err_style, "fatal: %s\n", err.what());
    die(exit_code::INVALID_ARGUMENT_SYNTAX);
  }
  bpo::notify(args);

  std::string const
    cfg_path = get_required_arg<std::string>("cfg", args, missing_arg_ec),
    sim_name = get_required_arg<std::string>("name", args, missing_arg_ec);

  u64 const generation_target = get_required_arg<u64>("maxgen", args, missing_arg_ec);
  if (generation_target == 0) {
    printf(err_style, "fatal: generation target must be > 0\n");
    die(exit_code::BAD_ARGUMENT_VALUE);
  }

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

  try {
    state = simulation::parse_state(
      util::extract_txt_file_contents(cfg_path.c_str(), false),
      std::filesystem::current_path(),
      errors
    );
  } catch (std::runtime_error const &except) {
    printf(err_style, "fatal: %s\n", except.what());
    die(exit_code::GENERIC_ERROR);
  }

  if (!errors.empty()) {
    printf(err_style, "fatal: %zu state errors (--cfg)\n", errors.size());
    for (auto const &err : errors)
      printf(err_style, "  %s\n", err.c_str());

    die(exit_code::BAD_STATE);
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

  std::string const save_dir = get_optional_arg<std::string>("savedir", args).value_or("");
  if (save_dir != "" && !fs::is_directory(save_dir)) {
    printf(err_style, "fatal: --savedir \"%s\" is not a directory\n", save_dir.c_str());
    die(exit_code::BAD_ARGUMENT_VALUE);
  }

  u64 const save_interval = get_optional_arg<u64>("saveinterval", args).value_or(0ui64);
  b8 const save_final_state = get_flag_arg("savefinalstate", args);

  std::thread sim_thread(
    simulation::run,
    std::ref(state),
    sim_name,
    generation_target,
    save_points,
    save_interval,
    img_fmt,
    save_dir,
    save_final_state
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

  return static_cast<int>(exit_code::SUCCESS);
}
