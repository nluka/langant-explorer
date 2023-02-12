#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>

#include <boost/program_options.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "prgopt.hpp"
#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using namespace term::color;
using json_t = nlohmann::json;
using string = std::string;
using util::die;
using util::strvec_t;
using util::make_str;
using prgopt::get_required_arg;
using prgopt::get_optional_arg;

enum class exit_code : int
{
  UNKNOWN_ERROR,
  GENERIC_ERROR,
  SUCCESS = 0,
  INVALID_ARGUMENT_SYNTAX,
  MISSING_REQUIRED_ARGUMENT,
  BAD_ARGUMENT_VALUE,
  FILE_NOT_FOUND,
  FILE_OPEN_FAILED,
  BAD_FILE,
  BAD_CONFIG,
};

char const *usage_msg()
{
  return
    "USAGE \n"
    "  gencfg <count> <minlen> <maxlen> <gw> <gh> <gs> <ac> <ar> \n"
    "  [td] [ao] [outdir] \n"
    "REQUIRED \n"
    "  count ..... number of configurations to generate \n"
    "  minlen .... minimum ruleset length, inclusive \n"
    "  maxlen .... maximum ruleset length, inclusive \n"
    "  gw ........ grid width, [1, 65535] \n"
    "  gh ........ grid height, [1, 65535] \n"
    "  gs ........ grid state \n"
    "  ac ........ initial ant row, [0, gw) \n"
    "  ar ........ initial ant row, [0, gh) \n"
    "OPTIONAL \n"
    "  outdir .... where to emit generated JSON files, default=cwd \n"
    "  td ........ string of possible turn direction values, default=LNR \n"
    "  ao ........ string of possible ant orientation values, default=NESW \n"
  ;
}

boost::program_options::options_description my_boost_options()
{
  bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()
    ("count", bpo::value<i64>(), empty_desc)
    ("minlen", bpo::value<i64>(), empty_desc)
    ("maxlen", bpo::value<i64>(), empty_desc)
    ("td", bpo::value<string>(), empty_desc)
    ("gw", bpo::value<i64>(), empty_desc)
    ("gh", bpo::value<i64>(), empty_desc)
    ("gs", bpo::value<string>(), empty_desc)
    ("ac", bpo::value<i64>(), empty_desc)
    ("ar", bpo::value<i64>(), empty_desc)
    ("ao", bpo::value<string>(), empty_desc)
    ("outdir", bpo::value<string>(), empty_desc)
  ;

  return desc;
}

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    puts(usage_msg());
    die(exit_code::MISSING_REQUIRED_ARGUMENT);
  }

  int const err_style = fore::RED | back::BLACK;

  // parse command line args
  bpo::variables_map args;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, my_boost_options()), args);
  } catch (std::exception const &err) {
    printf(err_style, "fatal: %s\n", err.what());
    die(exit_code::INVALID_ARGUMENT_SYNTAX);
  }
  bpo::notify(args);

  int const missing_arg_ec = static_cast<int>(exit_code::MISSING_REQUIRED_ARGUMENT);
  auto const count = get_required_arg<i64>("count", args, missing_arg_ec);
  auto const minlen = get_required_arg<i64>("minlen", args, missing_arg_ec);
  auto const maxlen = get_required_arg<i64>("maxlen", args, missing_arg_ec);
  auto const grid_width = get_required_arg<i64>("gw", args, missing_arg_ec);
  auto const grid_height = get_required_arg<i64>("gh", args, missing_arg_ec);
  auto const grid_state = get_required_arg<string>("gs", args, missing_arg_ec);
  auto const ant_row = get_required_arg<i64>("ac", args, missing_arg_ec);
  auto const ant_col = get_required_arg<i64>("ar", args, missing_arg_ec);

  auto const turn_dir_values = get_optional_arg<string>("td", args).value_or("LNR");
  auto const ant_orient_values = get_optional_arg<string>("ao", args).value_or("NESW");
  auto const out_dir = get_optional_arg<string>("outdir", args).value_or(".");

  // validate option values
  {
    strvec_t errors{};
    auto const add_err = [&errors](string &&err) {
      errors.emplace_back(err);
    };

    if (count < 1) {
      add_err("--count less than 1, must be at least 1");
    }

    {
      bool valid_minlen = true;
      if (minlen < 2 || minlen > 255) {
        add_err("--minlen not in range [2, 255]");
        valid_minlen = false;
      }

      if (maxlen < 2 || maxlen > 255) {
        add_err("--maxlen not in range [2, 255]");
      }

      if (valid_minlen && maxlen < minlen) {
        add_err("--maxlen cannot be less than --minlen");
      }
    }

    if (grid_width < 1 || grid_width > UINT16_MAX) {
      add_err(make_str("--gw not in range [1, %" PRIu16 "]", UINT16_MAX));
    } else if (!util::in_range_incl_excl(ant_col, 0i64, grid_width)) {
      add_err(make_str("--ac not in grid x-axis [0, %" PRId64 ")", grid_width));
    }

    if (grid_height < 1 || grid_height > UINT16_MAX) {
      add_err(make_str("--gh not in range [1, %" PRIu16 "]", UINT16_MAX));
    } else if (!util::in_range_incl_excl(ant_row, 0i64, grid_height)) {
      add_err(make_str("--ar not in grid y-axis [0, %" PRId64 ")", grid_width));
    }

    {
      std::regex const td_regex("^(L|N|R)+$");
      if (!std::regex_match(turn_dir_values, td_regex)) {
        add_err("--td contains illegal characters");
      } else {
        usize count_L = 0, count_N = 0, count_R = 0;
        for (char const ch : turn_dir_values) {
          switch (toupper(ch)) {
            case 'L': ++count_L; break;
            case 'N': ++count_N; break;
            case 'R': ++count_R; break;
          }
        }
        auto const validate_char_cnt = [&add_err](char const ch, usize const cnt) {
          if (cnt > 1)
            add_err(make_str("--td character '%c' appears multiple (%zu) times", ch, cnt));
        };
        validate_char_cnt('L', count_L);
        validate_char_cnt('N', count_N);
        validate_char_cnt('R', count_R);
      }
    }

    {
      std::regex const ao_regex("^(N|E|S|W)+$");
      if (!std::regex_match(ant_orient_values, ao_regex)) {
        add_err("--ao contains illegal characters");
      } else {
        usize count_N = 0, count_E = 0, count_S = 0, count_W = 0;
        for (char const ch : ant_orient_values) {
          switch (toupper(ch)) {
            case 'N': ++count_N; break;
            case 'E': ++count_E; break;
            case 'S': ++count_S; break;
            case 'W': ++count_W; break;
          }
        }
        auto const validate_char_cnt = [&add_err](char const ch, usize const cnt) {
          if (cnt > 1)
            add_err(make_str("--ao character '%c' appears multiple (%zu) times", ch, cnt));
        };
        validate_char_cnt('N', count_N);
        validate_char_cnt('E', count_E);
        validate_char_cnt('S', count_S);
        validate_char_cnt('W', count_W);
      }
    }

    if (out_dir != ".") {
      if (!fs::exists(out_dir))
        add_err(make_str("--outdir '%s' not found", out_dir.c_str()));
      else if (!fs::is_directory(out_dir))
        add_err(make_str("--outdir '%s' is not a directory", out_dir.c_str()));
    }

    if (!errors.empty()) {
      printf(err_style, "fatal: %zu errors:\n", errors.size());
      for (auto const &err : errors)
        printf(err_style, "  %s\n", err.c_str());
      die(exit_code::BAD_ARGUMENT_VALUE);
    }
  }

  std::random_device rand_device;
  std::mt19937 rand_num_gener(rand_device());
  std::uniform_int_distribution<usize> dist_rules_len(
    static_cast<usize>(minlen),
    static_cast<usize>(maxlen)
  );
  std::uniform_int_distribution<usize> dist_turn_dir(
    0ui64,
    turn_dir_values.length() - 1
  );
  std::uniform_int_distribution<usize> dist_ant_orient(
    0ui64,
    ant_orient_values.length() - 1
  );

  fs::path file_pathname = out_dir;
  file_pathname /= "blank.json";
  char buffer[256 + 1] { 0 };
  json_t cfg_json, temp_json;
  json_t::array_t rules_json;
  cfg_json["generation"] = 0;
  cfg_json["last_step_result"] = simulation::step_result::to_string(simulation::step_result::NIL);
  cfg_json["grid_width"] = grid_width;
  cfg_json["grid_height"] = grid_height;
  cfg_json["grid_state"] = grid_state;
  cfg_json["ant_col"] = ant_col;
  cfg_json["ant_row"] = ant_row;

  for (usize i = 0; i < static_cast<usize>(count); ++i) {
    {
      usize const rand_idx = dist_ant_orient(rand_num_gener);
      char const rand_ant_orient_str[] { ant_orient_values[rand_idx], '\0' };
      cfg_json["ant_orientation"] = rand_ant_orient_str;
    }

    usize const rules_len = static_cast<usize>(dist_rules_len(rand_num_gener));
    // fill buffer with `len` random turn directions
    for (usize j = 0; j < rules_len; ++j) {
      usize const rand_idx = dist_turn_dir(rand_num_gener);
      char const rand_turn_dir[] { turn_dir_values[rand_idx], '\0' };
      buffer[j] = *rand_turn_dir;
    }
    buffer[rules_len] = '\0';

    auto const add_rule = [buffer, &temp_json, &rules_json](
      usize const shade,
      usize const replacement,
      char const turn_dir
    ) {
      char const turn_dir_str[] { turn_dir, '\0' };

      temp_json["on"] = shade;
      temp_json["replace_with"] = replacement;
      temp_json["turn"] = turn_dir_str;

      rules_json.emplace_back(temp_json);
    };

    for (usize j = 0; j < rules_len - 1; ++j)
      add_rule(j, j + 1, buffer[j]);
    add_rule(rules_len - 1, 0, buffer[rules_len - 1]);

    cfg_json["rules"] = rules_json;

    // std::cout << make_str("len=%zu, %s\n", rules_len, buffer);
    // std::cout << cfg_json.dump(2) << '\n';

    file_pathname.replace_filename(buffer).replace_extension(".json");
    std::fstream file;
    try {
      file = util::open_file(file_pathname.string().c_str(), std::ios::out);
      file << cfg_json.dump(2);
    } catch (std::runtime_error const &except) {
      printf(err_style, "%s\n", except.what());
    }
  }

  return static_cast<int>(exit_code::SUCCESS);
}
