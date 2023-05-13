#include <filesystem>
#include <iostream>
#include <cinttypes>
#include <regex>
#include <random>
#include <unordered_set>

#include <boost/program_options.hpp>
#include <boost/container/static_vector.hpp>

#include "core_source_libs.hpp"
#include "simulation.hpp"

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

static u64 s_num_states_completed = 0;
static u64 s_num_states_failed = 0;
static u64 s_num_consecutive_states_failed = 0;
static u64 s_num_filename_conflicts = 0;

void write_progress_log(
  time_point_t start_time,
  std::optional<time_point_t> end_time);

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

  logger::set_stdout_logging(true);

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

  std::random_device rand_device;
  std::mt19937 rand_num_gener(rand_device());

  u64_distrib dist_rules_len(s_options.min_num_rules, s_options.max_num_rules);
  u64_distrib dist_turn_dir(u64(0), s_options.turn_directions.length() - 1);
  u64_distrib dist_ant_orient(u64(0), s_options.ant_orientations.length() - 1);

  // will be reused for each generated state file
  fs::path state_file_path = fs::path(s_options.out_dir_path) / "blank.json";
  // only used if name_mode is randwords
  std::string rand_fname{};

  std::string turn_dirs_buffer(256 + 1, '\0');
  simulation::rules_t rand_rules{};
  std::ofstream file;
  std::unordered_set<std::string> names{};
  u64 last_progress_log_iteration = 0;

  time_point_t const start_time = util::current_time();
  time_point_t last_progress_log_time = util::current_time();

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
        state_file_path.replace_filename(turn_dirs_buffer);
      } else {
        get_rand_name(rand_fname, rand_num_gener);
        state_file_path.replace_filename(rand_fname);
      }
      state_file_path.replace_extension(".json");

      if (names.contains(turn_dirs_buffer)) {
        ++s_num_filename_conflicts;
        continue;
      }

      std::string const state_file_path_str = state_file_path.generic_string();
      file.open(state_file_path_str.c_str(), std::ios::out);

      if (!file) {
        ++s_num_states_failed;
        ++s_num_consecutive_states_failed;
        continue;
      }

      b8 const write_success = simulation::print_state_json(
        file,
        state_file_path_str,
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

      if (write_success) {
        std::string const &name = turn_dirs_buffer;
        names.emplace(name);
        ++s_num_states_completed;
        s_num_consecutive_states_failed = 0;
      } else {
        ++s_num_states_failed;
        ++s_num_consecutive_states_failed;
        fs::remove(state_file_path); // remove state file since we failed to write it
      }

      file.close();

      // once every ~2 seconds, we print a progress update to stdout
      {
        time_point_t const now = util::current_time();

        u64 const
          nanos_since_last_progress_log = util::nanos_between(last_progress_log_time, now),
          millis_since_last_progress_log = nanos_since_last_progress_log / 1'000'000;

        if (millis_since_last_progress_log > 2000) {
          write_progress_log(start_time, std::nullopt);
          last_progress_log_time = util::current_time();
          last_progress_log_iteration = i;
        }
      }

    } catch (...) {
      ++s_num_states_failed;
      ++s_num_consecutive_states_failed;
    }

    if (s_num_consecutive_states_failed >= 3) {
      if (last_progress_log_iteration != i)
        write_progress_log(start_time, std::nullopt);

      die("%zu consecutive failures (did you run out of disk space?)", s_num_consecutive_states_failed);
    }
  }

  write_progress_log(start_time, util::current_time());

  return s_num_states_failed > 0;
}
catch (std::exception const &except)
{
  die("%s", except.what());
}
catch (...)
{
  die("unknown error");
}

void write_progress_log(
  time_point_t const start_time,
  std::optional<time_point_t> const end_time)
{
  using namespace term;
  using term::printf;

  static u8 const digits_in_num_states = util::count_digits(s_options.count);
  static char time_elapsed[64];

  // if the last state has been written, finish_time will be set and thus
  // we use it as the final timestamp for computing our statistics
  time_point_t const time_now = end_time.value_or(util::current_time());

  u64 const
    succeeded = s_num_states_completed,
    failed = s_num_states_failed,
    filename_conflicts = s_num_filename_conflicts,
    // total_done = succeeded + failed + filename_conflicts,
    total_nanos_elapsed = util::nanos_between(start_time, time_now);
  f64 const
    total_secs_elapsed = f64(total_nanos_elapsed) / 1'000'000'000.0,
    percent_done = ( f64(succeeded) / f64(s_options.count) ) * 100.0,
    states_per_sec = f64(succeeded) / total_secs_elapsed;

  util::time_span(u64(total_secs_elapsed)).stringify(time_elapsed, util::lengthof(time_elapsed));

  printf(FG_GREEN, "[%*zu/%zu] %6.2lf %%", digits_in_num_states, succeeded, s_options.count, percent_done);
  std::printf(", ");

  printf(FG_BRIGHT_BLUE, "%7.2lf states/s", states_per_sec);
  std::printf(", ");

  if (failed > 0) {
    printf(FG_RED, "%zu failed", failed);
    std::printf(", ");
  }

  if (filename_conflicts > 0) {
    printf(FG_RED, "%zu filename conflicts", filename_conflicts);
    std::printf(", ");
  }

  std::printf("%s elapsed\n", time_elapsed);
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
