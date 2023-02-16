#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>
// #include <boost/asio.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"
#include "lib/regexglob.hpp"
#include "lib/logger.hpp"

#ifdef _WIN32
#  include <Windows.h>
#  undef max // because it conflicts with std::max
#endif

#include "prgopt.hpp"
#include "primitives.hpp"
#include "timespan.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;
using namespace term::color;
using util::strvec_t;
using json_t = nlohmann::json;
using prgopt::get_required_arg;
using prgopt::get_optional_arg;
using prgopt::get_flag_arg;
using util::die;
using util::time_point_t;
using logger::event_type;

enum class exit_code : int
{
  SUCCESS = 0,
  GENERIC_ERROR,
  INVALID_ARGUMENT_SYNTAX,
  MISSING_REQUIRED_ARGUMENT,
  BAD_ARGUMENT_VALUE,
  BAD_STATE,
  UNKNOWN_ERROR,
};

char const *usage_msg()
{
  return
    "\n"
    "USAGE \n"
    "  simulate_many --states <path> --maxgen <int> \n"
    "                [--imgfmt <fmt>] [--savefinalstate] [--save_points <json_array>] \n"
    "                [--save_interval <int>] [--savedir <dir>] \n"
    "REQUIRED \n"
    "  --states .......... path to directory containing initial state JSON files \n"
    "  --maxgen .......... > 0 uint64, generation limit \n"
    "OPTIONAL \n"
    "  --imgfmt .......... rawPGM|plainPGM, default=rawPGM \n"
    "  --savefinalstate .. flag, guarantees final state is saved, off by default \n"
    "  --savepoints ...... JSON uint64 array, specific generations to save \n"
    "  --saveinterval .... > 0 uint64 \n"
    "  --savedir ......... path, where save files (.json, .pgm) are emitted, default=cwd \n"
  ;
}

bpo::options_description my_boost_options()
{
  bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()
    ("states", bpo::value<std::string>(), empty_desc)
    ("maxgen", bpo::value<u64>(), empty_desc)
    ("imgfmt", bpo::value<std::string>(), empty_desc)
    ("savefinalstate", /* flag */ empty_desc)
    ("savepoints", bpo::value<std::string>(), empty_desc)
    ("saveinterval", bpo::value<u64>(), empty_desc)
    ("savedir", bpo::value<std::string>(), empty_desc)
  ;

  return desc;
}

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    puts(usage_msg());
    die(exit_code::MISSING_REQUIRED_ARGUMENT);
  }

  logger::set_out_pathname("./.log");
  logger::set_autoflush(true);

  int const err_style = fore::RED | back::BLACK;
  int const missing_arg_ec = static_cast<int>(exit_code::MISSING_REQUIRED_ARGUMENT);

  // parse command line args
  bpo::variables_map args;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, my_boost_options()), args);
  } catch (std::exception const &err) {
    printf(err_style, "fatal: %s\n", err.what());
    die(exit_code::INVALID_ARGUMENT_SYNTAX);
  }
  bpo::notify(args);

  std::string const states_path = get_required_arg<std::string>("states", args, missing_arg_ec);

  u64 const generation_limit = get_required_arg<u64>("maxgen", args, missing_arg_ec);
  if (generation_limit == 0) {
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

  std::string const save_dir = get_optional_arg<std::string>("savedir", args).value_or(".");
  if (save_dir != "." && !fs::is_directory(save_dir)) {
    printf(err_style, "fatal: --savedir \"%s\" is not a directory\n", save_dir.c_str());
    die(exit_code::BAD_ARGUMENT_VALUE);
  }

  u64 const save_interval = get_optional_arg<u64>("saveinterval", args).value_or(0ui64);
  b8 const save_final_state = get_flag_arg("savefinalstate", args);

  auto const state_files = regexglob::fmatch(states_path.c_str(), ".*\\.json");
  if (state_files.empty()) {
    printf(err_style, "fatal: supplied --states directory has no JSON files\n");
    die(exit_code::BAD_ARGUMENT_VALUE);
  }

  struct named_simulation
  {
    std::string name;
    simulation::state state;
  };

  std::vector<named_simulation> simulations(state_files.size());
  strvec_t errors{};

  for (usize i = 0; i < state_files.size(); ++i) {
    std::string const state_path = state_files[i].string();
    auto &sim = simulations[i];

    try {
      sim.state = simulation::parse_state(
        util::extract_txt_file_contents(state_path.c_str(), false),
        state_files[i].relative_path(),
        errors
      );

      if (errors.empty()) {
        logger::log(event_type::INF, "parsed '%s' successfully", state_path.c_str());
      } else {
        logger::log(event_type::FTL, "failed to parse '%s'", state_path.c_str());

        printf(err_style, "fatal: failed to parse '%s', %zu errors:\n", state_path.c_str(), errors.size());
        for (auto const &err : errors)
          printf(err_style, "  %s\n", err.c_str());

        die(exit_code::BAD_STATE);
      }

      sim.name = fs::path(state_path).filename().replace_extension("").string();

    } catch (std::runtime_error const &except) {
      logger::log(event_type::FTL, "failed to parse '%s', %s", state_path.c_str(), except.what());
      printf(err_style, "fatal: failed to parse '%s', %s\n", state_path.c_str(), except.what());
      die(exit_code::GENERIC_ERROR);
    }
  }

  auto const simulation_task = [
    generation_limit,
    &save_points,
    save_interval,
    img_fmt,
    &save_dir
  ](
    named_simulation &sim
  ) {
    logger::log(event_type::INF, "started simulation %s", sim.name.c_str());

    auto const result = simulation::run(
      sim.state,
      sim.name,
      generation_limit,
      save_points,
      save_interval,
      img_fmt,
      save_dir,
      true
    );

    // log result
    logger::log(
      event_type::INF,
      "finished simulation %s, %s",
      sim.name.c_str(),
      (result.code == simulation::run_result::REACHED_GENERATION_LIMIT
        ? "reached generation limit"
        : "hit edge")
    );
  };

  for (auto &sim : simulations) {
    simulation_task(sim);
  }

  return static_cast<int>(exit_code::SUCCESS);
}
