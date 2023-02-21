#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>

#include <boost/program_options.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using json_t = nlohmann::json;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

typedef std::uniform_int_distribution<usize> usize_distrib;

#define OPT_COUNT_FULL "count"
#define OPT_COUNT_SHORT "n"

#define OPT_OUTPATH_FULL "outpath"
#define OPT_OUTPATH_SHORT "p"

#define OPT_NAMESTYLE_FULL "namestyle"
#define OPT_NAMESTYLE_SHORT "l"
#define REGEX_NAMESTYLE "^(turndirecs)|(randwords,[1-9])$"

#define OPT_RULESLEN_FULL "ruleslen"
#define OPT_RULESLEN_SHORT "r"
#define REGEX_RULESLEN "^[0-9]+,[0-9]+$"

#define OPT_TURNDIRECS_FULL "turndirecs"
#define OPT_TURNDIRECS_SHORT "t"
#define REGEX_TURNDIRECS "^[lLnNrR]+$"

#define OPT_GRIDWIDTH_FULL "gridwidth"
#define OPT_GRIDWIDTH_SHORT "w"

#define OPT_GRIDHEIGHT_FULL "gridheight"
#define OPT_GRIDHEIGHT_SHORT "h"

#define OPT_GRIDSTATE_FULL "gridstate"
#define OPT_GRIDSTATE_SHORT "g"

#define OPT_ANTCOL_FULL "antcol"
#define OPT_ANTCOL_SHORT "x"

#define OPT_ANTROW_FULL "antrow"
#define OPT_ANTROW_SHORT "y"

#define OPT_ANTORIENTS_FULL "antorients"
#define OPT_ANTORIENTS_SHORT "o"
#define REGEX_ANTORIENTS "^[nNeEsSwW]+$"

// TODO: add color shuffle feature
struct config
{
  std::string
    name_style,
    grid_state,
    turn_dir_values,
    ant_orient_values
  ;
  fs::path out_path;
  usize
    count,
    rules_min_len,
    rules_max_len,
    grid_width,
    grid_height,
    ant_col,
    ant_row
  ;
};

static config s_cfg{};

errors_t parse_config(int const argc, char const *const *argv);
bpo::options_description options_descrip();
std::string usage_msg();
simulation::rules_t make_random_rules(
  char *turn_dirs_buffer,
  usize_distrib const &dist_rules_len,
  usize_distrib const &dist_turn_dir,
  std::mt19937 &num_generator);
void get_rand_name(
  std::string &dest,
  std::mt19937 &num_generator,
  std::string const &words,
  std::vector<usize> const &newline_positions,
  usize num_rand_words,
  char name_word_delim);
usize ascii_digit_to_usize(char ch);

int main(int const argc, char const *const *const argv)
{
  if (argc == 1) {
    std::cout << usage_msg();
    std::exit(1);
  }

  {
    errors_t const errors = parse_config(argc, argv);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err(err.c_str());
      die("%zu configuration errors", errors.size());
    }
  }

  std::random_device rand_device;
  std::mt19937 rand_num_gener(rand_device());

  usize_distrib const dist_rules_len(s_cfg.rules_min_len, s_cfg.rules_max_len);
  usize_distrib const dist_turn_dir(0ui64, s_cfg.turn_dir_values.length() - 1);
  usize_distrib const dist_ant_orient(0ui64, s_cfg.ant_orient_values.length() - 1);

  // will be reused for each generated state file
  fs::path state_path = s_cfg.out_path / "blank.json";

  // only used if name_style is randwords
  std::string words{};
  std::vector<usize> words_newline_positions{};
  std::string rand_fname{};
  if (s_cfg.name_style.starts_with("randwords")) {
    char const *const words_file_path = "words.txt";
    try {
      words = util::extract_txt_file_contents(words_file_path, true);
    } catch (char const *const err) {
      die("failed to extract '%s', %s", words_file_path, err);
    }
    words_newline_positions.reserve(10);
    for (usize i = 0; i < words.length(); ++i)
      if (words[i] == '\n')
        words_newline_positions.push_back(i);
  }

  usize num_fname_conflicts = 0;

  for (usize i = 0; i < s_cfg.count; ++i) {
    simulation::orientation::value_type const rand_ant_orient = [&]() {
      usize const rand_idx = dist_ant_orient(rand_num_gener);
      char const rand_ant_orient_str[] {
        static_cast<char>(toupper(s_cfg.ant_orient_values[rand_idx])),
        '\0'
      };
      return simulation::orientation::from_string(rand_ant_orient_str);
    }();

    char turn_dirs_buffer[256 + 1] { 0 };
    simulation::rules_t const rand_rules = make_random_rules(
      turn_dirs_buffer,
      dist_rules_len,
      dist_turn_dir,
      rand_num_gener);

    if (s_cfg.name_style == "turndirecs") {
      state_path.replace_filename(turn_dirs_buffer);
    } else {
      get_rand_name(
        rand_fname,
        rand_num_gener,
        words,
        words_newline_positions,
        ascii_digit_to_usize(s_cfg.name_style.back()),
        '_');
      state_path.replace_filename(rand_fname);
    }
    state_path.replace_extension(".json");

    if (fs::exists(state_path)) {
      ++num_fname_conflicts;
      continue;
    }

    try {
      std::fstream file = util::open_file(state_path.string().c_str(), std::ios::out);
      simulation::print_state_json(
        file,
        0,
        simulation::step_result::NIL,
        static_cast<i32>(s_cfg.grid_width),
        static_cast<i32>(s_cfg.grid_height),
        s_cfg.grid_state,
        static_cast<i32>(s_cfg.ant_col),
        static_cast<i32>(s_cfg.ant_row),
        rand_ant_orient,
        rand_rules);
    } catch (std::runtime_error const &except) {
      print_err(except.what());
    }
  }

  std::cout
    << "generated " << (s_cfg.count - num_fname_conflicts) << " files, "
    << num_fname_conflicts << " filename conflicts\n";

  return 0;
}

std::string usage_msg()
{
  std::ostringstream msg;

  msg <<
    "\n"
    "USAGE: \n"
    "  genstate [options] \n"
    "\n"
  ;

  options_descrip().print(msg, 6);

  msg <<
    "\n"
    "NOTES: \n"
    "  For option -" OPT_ANTORIENTS_SHORT " [ --" OPT_ANTORIENTS_FULL " ], the value must match /" << REGEX_ANTORIENTS << "/. \n"
    "  If an orientation is repeated, it is more likely to occur. \n"
    "  For example: NNNEESW means N has a 3/7 chance, E has a 2/7 chance, \n"
    "  and S|W have a 1/7 chance of being randomly selected. \n"
    "  The same concept applies for -" OPT_TURNDIRECS_SHORT " [ --" OPT_TURNDIRECS_FULL " ], but the value must match /" << REGEX_TURNDIRECS << "/. \n"
    "\n"
  ;

  return msg.str();
}

bpo::options_description options_descrip()
{
  bpo::options_description desc("REQUIRED OPTIONS");

  desc.add_options()
    (OPT_COUNT_FULL "," OPT_COUNT_SHORT, bpo::value<u64>(), "number of randomized states to generate")
    (OPT_OUTPATH_FULL "," OPT_OUTPATH_SHORT, bpo::value<std::string>(), "directory in which to save generated states (.json)")
    (OPT_NAMESTYLE_FULL "," OPT_NAMESTYLE_SHORT, bpo::value<std::string>(), "naming style for generated state filenames, format /" REGEX_NAMESTYLE "/, default=turndirecs")
    // rules
    (OPT_RULESLEN_FULL "," OPT_RULESLEN_SHORT, bpo::value<std::string>(), "[min, max] rules length, format /" REGEX_RULESLEN "/")
    (OPT_TURNDIRECS_FULL "," OPT_TURNDIRECS_SHORT, bpo::value<std::string>(), "possible rule turn directions, format /" REGEX_TURNDIRECS "/, see notes for details")
    // grid
    (OPT_GRIDWIDTH_FULL "," OPT_GRIDWIDTH_SHORT, bpo::value<u64>(), "grid_width value for all generated states, [1, 65535]")
    (OPT_GRIDHEIGHT_FULL "," OPT_GRIDHEIGHT_SHORT, bpo::value<u64>(), "grid_height value for all generated states, [1, 65535]")
    (OPT_GRIDSTATE_FULL "," OPT_GRIDSTATE_SHORT, bpo::value<std::string>(), "grid_state for all generated states, any string")
    // ant
    (OPT_ANTCOL_FULL "," OPT_ANTCOL_SHORT, bpo::value<u64>(), "ant_col value for all generated states, [0, grid_width)")
    (OPT_ANTROW_FULL "," OPT_ANTROW_SHORT, bpo::value<u64>(), "ant_row value for all generated states, [0, grid_height)")
    (OPT_ANTORIENTS_FULL "," OPT_ANTORIENTS_SHORT, bpo::value<std::string>(), "possible ant_orientation values, format /" REGEX_ANTORIENTS "/, see notes for details")
  ;

  return desc;
}

errors_t parse_config(int const argc, char const *const *const argv)
{
  using util::get_required_option;
  using util::get_nonrequired_option;
  using util::make_str;

  errors_t errors{};

  bpo::variables_map var_map;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, options_descrip()), var_map);
  } catch (std::exception const &err) {
    errors.emplace_back(err.what());
    return errors;
  }
  bpo::notify(var_map);

  {
    auto name_style = get_nonrequired_option<std::string>(OPT_NAMESTYLE_FULL, OPT_NAMESTYLE_SHORT, var_map, errors);
    if (name_style.has_value()) {
      if (std::regex_match(name_style.value(), std::regex(REGEX_NAMESTYLE))) {
        s_cfg.name_style = std::move(name_style.value());
      } else {
        errors.emplace_back("(--" OPT_NAMESTYLE_FULL ", -" OPT_NAMESTYLE_SHORT ") must match /" REGEX_NAMESTYLE "/");
      }
    } else {
      s_cfg.name_style = "turndirecs";
    }
  }
  {
    // no validation for grid_state
    s_cfg.grid_state = get_required_option<std::string>(OPT_GRIDSTATE_FULL, OPT_GRIDSTATE_SHORT, var_map, errors).value_or("");
  }
  {
    auto turn_dirs = get_required_option<std::string>(OPT_TURNDIRECS_FULL, OPT_TURNDIRECS_SHORT, var_map, errors);

    if (turn_dirs.has_value()) {
      if (!std::regex_match(turn_dirs.value(), std::regex(REGEX_TURNDIRECS))) {
        errors.emplace_back("(--" OPT_TURNDIRECS_FULL ", -" OPT_TURNDIRECS_SHORT ") must match /" REGEX_TURNDIRECS "/");
      } else {
        s_cfg.turn_dir_values = std::move(turn_dirs.value());
      }
    }
  }
  {
    auto ant_orients = get_required_option<std::string>(OPT_ANTORIENTS_FULL, OPT_ANTORIENTS_SHORT, var_map, errors);

    if (ant_orients.has_value()) {
      if (!std::regex_match(ant_orients.value(), std::regex(REGEX_ANTORIENTS))) {
        errors.emplace_back("(--" OPT_ANTORIENTS_FULL ", -" OPT_ANTORIENTS_SHORT ") must match /" REGEX_ANTORIENTS "/");
      } else {
        s_cfg.ant_orient_values = std::move(ant_orients.value());
      }
    }
  }
  {
    auto out_path = get_required_option<std::string>(OPT_OUTPATH_FULL, OPT_OUTPATH_SHORT, var_map, errors);

    if (out_path.has_value()) {
      if (!fs::exists(out_path.value())) {
        bool const create_dir = util::user_wants_to_create_dir(out_path.value());
        if (create_dir) {
          try {
            fs::create_directories(out_path.value());
            s_cfg.out_path = std::move(out_path.value());
          } catch (fs::filesystem_error const &except) {
            print_err("unable to create directory: %s", except.what());
          }
        } else {
          errors.emplace_back("(--" OPT_OUTPATH_FULL ", -" OPT_OUTPATH_SHORT ") path not found");
        }
      } else if (!fs::is_directory(out_path.value())) {
        errors.emplace_back("(--" OPT_OUTPATH_FULL ", -" OPT_OUTPATH_SHORT ") path is not a directory");
      } else {
        s_cfg.out_path = std::move(out_path.value());
      }
    }
  }
  {
    auto const count = get_required_option<usize>(OPT_COUNT_FULL, OPT_COUNT_SHORT, var_map, errors);

    if (count.has_value()) {
      if (count.value() < 1) {
        errors.emplace_back("(--" OPT_COUNT_FULL ", -" OPT_COUNT_SHORT ") must > 0");
      } else {
        s_cfg.count = count.value();
      }
    }
  }
  {
    auto rules_len = get_required_option<std::string>(OPT_RULESLEN_FULL, OPT_RULESLEN_SHORT, var_map, errors);

    if (rules_len.has_value()) {
      if (!std::regex_match(rules_len.value(), std::regex(REGEX_RULESLEN))) {
        errors.emplace_back("(--" OPT_RULESLEN_FULL ", -" OPT_RULESLEN_SHORT ") must match /" REGEX_RULESLEN "/");
      } else {
        // since we verified the format of `rules_len_str`,
        // there shouldn't be any exceptions caused by this block...

        usize const comma_pos = rules_len.value().find_first_of(',');
        assert(comma_pos != std::string::npos);
        rules_len.value()[comma_pos] = '\0';

        char const *const min_cstr = rules_len.value().c_str();
        char const *const max_cstr = min_cstr + comma_pos + 1;

        usize const min = std::stoull(min_cstr);
        usize const max = std::stoull(max_cstr);

        if (min < 2 || max > 255) {
          errors.emplace_back("(--" OPT_RULESLEN_FULL ", -" OPT_RULESLEN_SHORT ") value(s) not in range [2, 255]");
        } else if (max < min) {
          errors.emplace_back("(--" OPT_RULESLEN_FULL ", -" OPT_RULESLEN_SHORT ") max(rhs) must be >= min(lhs)");
        } else {
          s_cfg.rules_min_len = min;
          s_cfg.rules_max_len = max;
        }
      }
    }
  }
  {
    auto const grid_width = get_required_option<usize>(OPT_GRIDWIDTH_FULL, OPT_GRIDWIDTH_SHORT, var_map, errors);
    auto const ant_col = get_required_option<usize>(OPT_ANTCOL_FULL, OPT_ANTCOL_SHORT, var_map, errors);

    if (grid_width.has_value() && ant_col.has_value()) {
      if (grid_width.value() < 1 || grid_width.value() > UINT16_MAX) {
        errors.emplace_back(make_str("(--" OPT_GRIDWIDTH_FULL ", -" OPT_GRIDWIDTH_SHORT ") not in range [1, %zu]", UINT16_MAX));
      } else if (!util::in_range_incl_excl<usize>(ant_col.value(), 0, grid_width.value())) {
        errors.emplace_back(make_str("(--" OPT_ANTCOL_FULL ", -" OPT_ANTCOL_SHORT ") not on grid x-axis [0, %zu)", grid_width.value()));
      } else {
        s_cfg.grid_width = grid_width.value();
        s_cfg.ant_col = ant_col.value();
      }
    }
  }
  {
    auto const grid_height = get_required_option<usize>(OPT_GRIDHEIGHT_FULL, OPT_GRIDHEIGHT_SHORT, var_map, errors);
    auto const ant_row = get_required_option<usize>(OPT_ANTROW_FULL, OPT_ANTROW_SHORT, var_map, errors);

    if (grid_height.has_value() && ant_row.has_value()) {
      if (grid_height.value() < 1 || grid_height.value() > UINT16_MAX) {
        errors.emplace_back(make_str("(--" OPT_GRIDHEIGHT_FULL ", -" OPT_GRIDHEIGHT_SHORT ") not in range [1, %zu]", UINT16_MAX));
      } else if (!util::in_range_incl_excl<usize>(ant_row.value(), 0, grid_height.value())) {
        errors.emplace_back(make_str("(--" OPT_ANTROW_FULL ", -" OPT_ANTROW_SHORT ") not on grid y-axis [0, %zu)", grid_height.value()));
      } else {
        s_cfg.grid_height = grid_height.value();
        s_cfg.ant_row = ant_row.value();
      }
    }
  }

  return std::move(errors);
}

simulation::rules_t make_random_rules(
  char *const turn_dirs_buffer,
  usize_distrib const &dist_rules_len,
  usize_distrib const &dist_turn_dir,
  std::mt19937 &num_generator)
{
  simulation::rules_t rules{};

  usize const rules_len = dist_rules_len(num_generator);

  // fill `turn_dirs_buffer` with `rules_len` random turn directions
  for (usize i = 0; i < rules_len; ++i) {
    usize const rand_idx = dist_turn_dir(num_generator);
    char const rand_turn_dir = static_cast<char>(toupper(s_cfg.turn_dir_values[rand_idx]));
    turn_dirs_buffer[i] = rand_turn_dir;
  }
  turn_dirs_buffer[rules_len] = '\0';

  usize const last_rule_idx = rules_len - 1;
  for (usize i = 0; i < last_rule_idx; ++i) {
    rules[i].turn_dir = simulation::turn_direction::from_char(turn_dirs_buffer[i]);
    rules[i].replacement_shade = static_cast<u8>(i + 1);
  }
  rules[last_rule_idx].turn_dir = simulation::turn_direction::from_char(turn_dirs_buffer[last_rule_idx]);
  rules[last_rule_idx].replacement_shade = 0;

  return rules;
}

void get_rand_name(
  std::string &dest,
  std::mt19937 &num_generator,
  std::string const &words,
  std::vector<usize> const &newline_positions,
  usize const num_rand_words,
  char const name_word_delim)
{
  dest.clear();

  auto const pick_rand_word = [&]() -> std::string_view {
    usize_distrib const distrib(0, newline_positions.size() - 2);
    // size-2 because we don't want to pick the very last newline
    // since we create the view going forwards from the chosen newline
    // to the next newline.

    usize const rand_newline_idx = distrib(num_generator);
    usize const next_newline_idx = rand_newline_idx + 1;

    usize const start_pos = newline_positions[rand_newline_idx] + 1;
    usize const count = newline_positions[next_newline_idx] - start_pos;

    return { words.c_str() + start_pos, count };
  };

  for (usize i = 0; i < num_rand_words - 1; ++i) {
    dest.append(pick_rand_word());
    dest += name_word_delim;
  }
  dest.append(pick_rand_word());
}

// Converts an ASCII digit ('0'-'9') to an integer value (0-9).
usize ascii_digit_to_usize(char const ch)
{
  assert(ch >= '0' && ch <= '9');
  return static_cast<usize>(ch) - 48;
}
