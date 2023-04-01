#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>

#include <boost/program_options.hpp>
#include <boost/container/static_vector.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"
#include "program_options.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using json_t = nlohmann::json;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

typedef std::uniform_int_distribution<usize> usize_distrib;

static std::string s_words{};
static std::vector<usize> s_words_newline_positions{};
static po::make_states_options s_options{};

simulation::rules_t make_random_rules(
  char *turn_dirs_buffer,
  usize_distrib const &dist_rules_len,
  usize_distrib const &dist_turn_dir,
  std::mt19937 &num_generator);

void get_rand_name(
  std::string &dest,
  std::mt19937 &num_generator);

int main(int const argc, char const *const *const argv)
{
  if (argc < 2) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "USAGE:\n"
      "  make_states [options]\n"
      "\n";
    po::make_states_options_description().print(usage_msg, 6);
    usage_msg << '\n';
    std::cout << usage_msg.str();
    std::exit(1);
  }

  {
    errors_t errors{};
    po::parse_make_states_options(argc, argv, s_options, errors);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      die("%zu configuration errors", errors.size());
    }
  }

  // initialization for when name_mode is randwords
  if (s_options.name_mode.starts_with("randwords")) {
    try {
      s_words = util::extract_txt_file_contents(s_options.word_file_path, true);
    } catch (char const *const err) {
      die("failed to extract '%s', %s", s_options.word_file_path.c_str(), err);
    }

    s_words_newline_positions.reserve(10);
    for (usize i = 0; i < s_words.length(); ++i)
      if (s_words[i] == '\n')
        s_words_newline_positions.push_back(i);
  }

  std::random_device rand_device;
  std::mt19937 rand_num_gener(rand_device());

  usize_distrib const dist_rules_len(s_options.min_num_rules, s_options.max_num_rules);
  usize_distrib const dist_turn_dir(0ui64, s_options.turn_directions.length() - 1);
  usize_distrib const dist_ant_orient(0ui64, s_options.ant_orientations.length() - 1);

  usize num_fname_conflicts = 0;
  // will be reused for each generated state file
  fs::path state_path = fs::path(s_options.out_dir_path) / "blank.json";
  // only used if name_mode is randwords
  std::string rand_fname{};

  for (usize i = 0; i < s_options.count; ++i) {
    simulation::orientation::value_type const rand_ant_orient = [&]() {
      usize const rand_idx = dist_ant_orient(rand_num_gener);
      char const rand_ant_orient_str[] {
        static_cast<char>(toupper(s_options.ant_orientations[rand_idx])),
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

    if (s_options.name_mode == "turndirecs") {
      state_path.replace_filename(turn_dirs_buffer);
    } else if (s_options.name_mode.starts_with("alpha")) {

    } else {
      get_rand_name(rand_fname, rand_num_gener);
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
        s_options.grid_width,
        s_options.grid_height,
        s_options.grid_state,
        s_options.ant_col,
        s_options.ant_row,
        rand_ant_orient,
        rand_rules);
    } catch (std::runtime_error const &except) {
      print_err(except.what());
    }
  }

  std::cout << "Generated " << (s_options.count - num_fname_conflicts) << " randomized states";
  if (num_fname_conflicts > 0)
    std::cout << " (" << num_fname_conflicts << " filename conflicts)";
  std::cout << '\n';

  return 0;
}

simulation::rules_t make_random_rules(
  char *const turn_dirs_buffer,
  usize_distrib const &dist_rules_len,
  usize_distrib const &dist_turn_dir,
  std::mt19937 &num_generator)
{
  simulation::rules_t rules{};

  usize const rules_len = dist_rules_len(num_generator);
  assert(rules_len >= 2);
  assert(rules_len <= 256);

  // fill `turn_dirs_buffer` with `rules_len` random turn directions
  for (usize i = 0; i < rules_len; ++i) {
    usize const rand_idx = dist_turn_dir(num_generator);
    char const rand_turn_dir = static_cast<char>(toupper(s_options.turn_directions[rand_idx]));
    turn_dirs_buffer[i] = rand_turn_dir;
  }
  turn_dirs_buffer[rules_len] = '\0';

  usize const last_rule_idx = rules_len - 1;

  auto const set_rule = [&](
    usize const shade,
    usize const replacement_shade)
  {
    rules[shade].turn_dir = simulation::turn_direction::from_char(turn_dirs_buffer[shade]);
    rules[shade].replacement_shade = static_cast<u8>(replacement_shade);
  };

  if (s_options.shade_order == "asc") {
    // 0->1
    // 1->2  0->1->2->3
    // 2->3  ^--------'
    // 3->0

    for (usize i = 0; i < last_rule_idx; ++i)
      set_rule(i, i + 1);
    set_rule(last_rule_idx, 0);

  } else if (s_options.shade_order == "desc") {
    // 0->3
    // 1->0  3->2->1->0
    // 2->1  ^--------'
    // 3->2

    for (usize i = last_rule_idx; i > 0; --i)
      set_rule(i, i - 1);
    set_rule(0, last_rule_idx);

  } else { // rand
    boost::container::static_vector<u8, 256> remaining_shades{};
    for (usize shade = 0; shade < rules_len; ++shade)
      remaining_shades.push_back(static_cast<u8>(shade));

    boost::container::static_vector<u8, 256> chain{};
    // [0]->[1]->[2]->[3] ...indices
    //  ^--------------'

    for (usize i = 0; i < rules_len; ++i) {
      usize_distrib const remaining_idx_dist(0, remaining_shades.size() - 1);
      usize const rand_shade_idx = remaining_idx_dist(num_generator);
      u8 const rand_shade = remaining_shades[rand_shade_idx];
      {
        // remove chosen shade by swapping it with the last shade, then popping
        u8 const temp = remaining_shades.back();
        remaining_shades.back() = rand_shade;
        remaining_shades[rand_shade_idx] = temp;
        remaining_shades.pop_back();
      }
      chain.push_back(rand_shade);
    }

    for (auto shade_it = chain.begin(); shade_it < chain.end() - 1; ++shade_it)
      set_rule(*shade_it, *(shade_it + 1));
    set_rule(chain.back(), chain.front());
  }

  return rules;
}

void get_rand_name(
  std::string &dest,
  std::mt19937 &num_generator)
{
  dest.clear();

  enum class name_mode
  {
    ALPHA,
    WORDS,
  };

  static name_mode const s_mode = [] {
    if (s_options.name_mode.starts_with("alpha"))
      return name_mode::ALPHA;
    else
      return name_mode::WORDS;
  }();

  static usize const s_num_elements = util::ascii_digit_to<usize>(s_options.name_mode.back());

  switch (s_mode) {
    case name_mode::WORDS: {
      auto const pick_rand_word = [&]() -> std::string_view {
        usize_distrib const distrib(0, s_words_newline_positions.size() - 2);
        // size-2 because we don't want to pick the very last newline
        // since we create the view going forwards from the chosen newline
        // to the next newline.

        usize const rand_newline_idx = distrib(num_generator);
        usize const next_newline_idx = rand_newline_idx + 1;

        usize const start_pos = s_words_newline_positions[rand_newline_idx] + 1;
        usize const count = s_words_newline_positions[next_newline_idx] - start_pos;

        return { s_words.c_str() + start_pos, count };
      };

      for (usize i = 0; i < s_num_elements - 1; ++i) {
        dest.append(pick_rand_word());
        dest += '_';
      }
      dest.append(pick_rand_word());

      break;
    }

    default:
    case name_mode::ALPHA: {
      static char const s_alphabet[] = "abcdefghijklmnopqrstuvwxyz"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

      usize_distrib const distrib_alpha(0, util::lengthof(s_alphabet) - 1);

      for (usize i = 0; i < s_num_elements - 1; ++i) {
        usize const rand_char_idx = distrib_alpha(num_generator);
        char const rand_char = s_alphabet[rand_char_idx];
        dest += rand_char;
      }

      break;
    }
  }
}