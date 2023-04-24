#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>
#include <unordered_set>

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
using util::time_point_t;
using util::make_str;
using util::print_err;
using util::die;

typedef std::uniform_int_distribution<u64> u64_distrib;

static std::string s_words{};
static std::vector<u64> s_words_newline_positions{};
static po::make_states_options s_options{};
static std::atomic<u64> s_num_states_completed = 0;
static std::atomic<u64> s_num_states_failed = 0;
static std::atomic<u64> s_num_filename_conflicts = 0;

simulation::rules_t make_random_rules(
  std::string &turn_dirs_buffer,
  u64_distrib &dist_rules_len,
  u64_distrib &dist_turn_dir,
  std::mt19937 &num_generator);

void get_rand_name(
  std::string &dest,
  std::mt19937 &num_generator);

i32 main(i32 const argc, char const *const *const argv)
try
{
  if (argc < 2) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "Usage:\n"
      "  make_states [options]\n"
      "\n";
    po::make_states_options_description().print(usage_msg, 6);
    usage_msg << '\n';
    std::cout << usage_msg.str();
    return 1;
  }

  {
    errors_t errors{};
    po::parse_make_states_options(argc, argv, s_options, errors);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      return 1;
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
    for (u64 i = 0; i < s_words.length(); ++i)
      if (s_words[i] == '\n')
        s_words_newline_positions.push_back(i);
  }

  std::thread states_writer([&]()
  {
    std::random_device rand_device;
    std::mt19937 rand_num_gener(rand_device());

    u64_distrib dist_rules_len(s_options.min_num_rules, s_options.max_num_rules);
    u64_distrib dist_turn_dir(u64(0), s_options.turn_directions.length() - 1);
    u64_distrib dist_ant_orient(u64(0), s_options.ant_orientations.length() - 1);

    // will be reused for each generated state file
    fs::path state_path = fs::path(s_options.out_dir_path) / "blank.json";
    // only used if name_mode is randwords
    std::string rand_fname{};

    std::string turn_dirs_buffer(256 + 1, '\0');
    simulation::rules_t rand_rules{};
    std::ofstream file;
    std::unordered_set<std::string> names{};

    for (u64 i = 0; i < s_options.count; ++i) {
      try {
        simulation::orientation::value_type const rand_ant_orient = [&] {
          u64 const rand_idx = dist_ant_orient(rand_num_gener);
          char const rand_ant_orient_str[] {
            static_cast<char>(toupper(s_options.ant_orientations[rand_idx])),
            '\0'
          };
          return simulation::orientation::from_cstr(rand_ant_orient_str);
        }();

        rand_rules = make_random_rules(
          turn_dirs_buffer,
          dist_rules_len,
          dist_turn_dir,
          rand_num_gener);

        if (s_options.name_mode == "turndirecs") {
          state_path.replace_filename(turn_dirs_buffer);
        } else {
          get_rand_name(rand_fname, rand_num_gener);
          state_path.replace_filename(rand_fname);
        }
        state_path.replace_extension(".json");

        if (names.contains(turn_dirs_buffer)) {
          ++s_num_filename_conflicts;
          continue;
        }

        file.open(state_path.string().c_str(), std::ios::out);

        simulation::print_state_json(
          file,
          s_options.grid_state,
          0,
          s_options.grid_width,
          s_options.grid_height,
          s_options.ant_col,
          s_options.ant_row,
          simulation::step_result::NIL,
          rand_ant_orient,
          util::count_digits(s_options.max_num_rules - 1),
          rand_rules);

        file.close();

        {
          std::string const &name = turn_dirs_buffer;
          names.emplace(name);
        }

        ++s_num_states_completed;

      } catch (std::runtime_error const &) {
        ++s_num_states_failed;
        // print_err(except.what());
      } catch (...) {
        ++s_num_states_failed;
      }
    }
  });

  term::cursor::hide();
  std::atexit(term::cursor::show);

  std::string const horizontal_rule(30, '-');
  time_point_t const start_time = util::current_time();

  // UI loop
  while (true) {
    using namespace term::color;

    time_point_t const time_now = util::current_time();

    u64 const
      completed = s_num_states_completed.load(),
      failed = s_num_states_failed.load(),
      filename_conflicts = s_num_filename_conflicts.load(),
      total_done = completed + failed + filename_conflicts,
      total_nanos_elapsed = util::nanos_between(start_time, time_now);
    f64 const
      total_secs_elapsed = total_nanos_elapsed / 1'000'000'000.0,
      percent_done = (total_done / static_cast<f64>(s_options.count)) * 100.0,
      states_per_sec = completed / total_secs_elapsed;

    u64 num_lines_printed = 0;

    auto const print_line = [&num_lines_printed](std::function<void ()> const &func) {
      func();
      term::clear_to_end_of_line();
      putc('\n', stdout);
      ++num_lines_printed;
    };

    print_line([&] { printf("%s", horizontal_rule.c_str()); });

    print_line([&] {
      printf("Progress   : ");
      printf("%.1lf %%", percent_done);
    });

    print_line([&] {
      printf("Completed  : ");
      printf(fore::LIGHT_GREEN | back::BLACK, "%zu", completed);
    });

    print_line([&] {
      printf("States/sec : ");
      printf(fore::WHITE | back::MAGENTA, "%.2lf", states_per_sec);
    });

    print_line([&] {
      printf("Failed     : ");
      printf(fore::LIGHT_RED | back::BLACK, "%zu", failed + filename_conflicts);
      if (filename_conflicts > 0)
        printf(" (%zu name conflicts)", filename_conflicts);
    });

    print_line([&] { printf("Elapsed    : %s",
      util::time_span(static_cast<u64>(total_secs_elapsed)).to_string().c_str()); });

    print_line([&] { printf("%s", horizontal_rule.c_str()); });

    if (total_done == s_options.count) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    term::cursor::move_up(num_lines_printed);
  }

  states_writer.join();

  return 0;
}
catch (std::exception const &except)
{
  die("%s", except.what());
}
catch (...)
{
  die("unknown error");
}

simulation::rules_t make_random_rules(
  std::string &turn_dirs_buffer,
  u64_distrib &dist_rules_len,
  u64_distrib &dist_turn_dir,
  std::mt19937 &num_generator)
{
  simulation::rules_t rules{};

  u64 const rules_len = dist_rules_len(num_generator);
  assert(rules_len >= 2);
  assert(rules_len <= 256);

  // fill `turn_dirs_buffer` with `rules_len` random turn directions
  turn_dirs_buffer.clear();
  for (u64 i = 0; i < rules_len; ++i) {
    u64 const rand_idx = dist_turn_dir(num_generator);
    char const rand_turn_dir = static_cast<char>(toupper(s_options.turn_directions[rand_idx]));
    turn_dirs_buffer += rand_turn_dir;
  }

  u64 const last_rule_idx = rules_len - 1;

  auto const set_rule = [&](
    u64 const shade,
    u64 const replacement_shade)
  {
    rules[shade].turn_dir = simulation::turn_direction::from_char(turn_dirs_buffer[shade]);
    rules[shade].replacement_shade = static_cast<u8>(replacement_shade);
  };

  if (s_options.shade_order == "asc") {
    // 0->1
    // 1->2  0->1->2->3
    // 2->3  ^--------'
    // 3->0

    for (u64 i = 0; i < last_rule_idx; ++i)
      set_rule(i, i + 1);
    set_rule(last_rule_idx, 0);

  } else if (s_options.shade_order == "desc") {
    // 0->3
    // 1->0  3->2->1->0
    // 2->1  ^--------'
    // 3->2

    for (u64 i = last_rule_idx; i > 0; --i)
      set_rule(i, i - 1);
    set_rule(0, last_rule_idx);

  } else { // rand
    boost::container::static_vector<u8, 256> remaining_shades{};
    for (u64 shade = 0; shade < rules_len; ++shade)
      remaining_shades.push_back(static_cast<u8>(shade));

    boost::container::static_vector<u8, 256> chain{};
    // [0]->[1]->[2]->[3] ...indices
    //  ^--------------'

    for (u64 i = 0; i < rules_len; ++i) {
      u64_distrib remaining_idx_dist(0, remaining_shades.size() - 1);
      u64 const rand_shade_idx = remaining_idx_dist(num_generator);
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

  static u64 const s_num_elements = util::ascii_digit_to<u64>(s_options.name_mode.back());

  switch (s_mode) {
    case name_mode::WORDS: {
      auto const pick_rand_word = [&]() -> std::string_view
      {
        u64_distrib distrib(0, s_words_newline_positions.size() - 2);
        // size-2 because we don't want to pick the very last newline
        // since we create the view going forwards from the chosen newline
        // to the next newline.

        u64 const rand_newline_idx = distrib(num_generator);
        u64 const next_newline_idx = rand_newline_idx + 1;

        u64 const start_pos = s_words_newline_positions[rand_newline_idx] + 1;
        u64 const count = s_words_newline_positions[next_newline_idx] - start_pos;

        return { s_words.c_str() + start_pos, count };
      };

      for (u64 i = 0; i < s_num_elements - 1; ++i) {
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

      u64_distrib distrib_alpha(0, util::lengthof(s_alphabet) - 2);

      for (u64 i = 0; i < s_num_elements - 1; ++i) {
        u64 const rand_char_idx = distrib_alpha(num_generator);
        char const rand_char = s_alphabet[rand_char_idx];
        dest += rand_char;
      }

      break;
    }
  }
}
