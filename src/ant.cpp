#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <regex>
#include <unordered_map>

#include "ant.hpp"
#include "json.hpp"
#include "util.hpp"

static
std::string error(char const *const fmt, ...) {
  static char buffer[1024] { 0 };

  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  return std::string(buffer);
}

static
u8 compute_maxval(
  u8 const *const arr,
  usize const width,
  usize const height
) {
  auto const *max = &arr[0];
  usize const len = width * height;
  for (usize i = 1; i < len; ++i) {
    if (arr[i] > *max) {
      max = &arr[i];
    }
  }
  return *max;
}

char const *ant::orientation::to_string(i8 const orient) {
  switch (orient) {
    case orientation::NORTH:
      return "north";
    case orientation::EAST:
      return "east";
    case orientation::SOUTH:
      return "south";
    case orientation::WEST:
      return "west";

    default:
    case orientation::OVERFLOW_CLOCKWISE:
    case orientation::OVERFLOW_COUNTER_CLOCKWISE:
      throw std::runtime_error("orientation::to_string failed - bad value");
  }
}

char const *ant::turn_direction::to_string(i8 const turn_dir) {
  switch (turn_dir) {
    case turn_direction::NIL:
      return "nil";
    case turn_direction::LEFT:
      return "L";
    case turn_direction::NONE:
      return "N";
    case turn_direction::RIGHT:
      return "R";

    default:
      throw std::runtime_error("turn_direction::to_string failed - bad value");
  }
}

char const *ant::step_result::to_string(step_result::type const step_res) {
  switch (step_res) {
    case step_result::NIL:
      return "nil";
    case step_result::SUCCESS:
      return "success";
    case step_result::FAILED_AT_BOUNDARY:
      return "failed_at_boundary";
    default:
      throw std::runtime_error("step_result::to_string failed - bad value");
  }
}

bool ant::rule::operator!=(ant::rule const &other) const noexcept {
  return this->replacement_shade != other.replacement_shade
    || this->turn_dir != other.turn_dir;
}

namespace ant {
  std::ostream &operator<<(std::ostream &os, ant::rule const &rule) {
    os << "(r=" << std::to_string(rule.replacement_shade)
      << ", d=" << ant::turn_direction::to_string(rule.turn_dir) << ')';
    return os;
  }
}

ant::step_result::type ant::simulation_step_forward(simulation &sim) {
  usize const curr_cell_idx = (sim.ant_row * sim.grid_width) + sim.ant_col;
  u8 const curr_cell_shade = sim.grid[curr_cell_idx];
  auto const &curr_cell_rule = sim.rules[curr_cell_shade];

  // turn
  sim.ant_orientation = sim.ant_orientation + curr_cell_rule.turn_dir;
  if (sim.ant_orientation == orientation::OVERFLOW_COUNTER_CLOCKWISE)
    sim.ant_orientation = orientation::WEST;
  else if (sim.ant_orientation == orientation::OVERFLOW_CLOCKWISE)
    sim.ant_orientation = orientation::NORTH;

  // update current cell shade
  sim.grid[curr_cell_idx] = curr_cell_rule.replacement_shade;

  #if 1

  int next_col, next_row;
  if (sim.ant_orientation == orientation::NORTH) {
    next_col = sim.ant_col;
    next_row = sim.ant_row - 1;
  } else if (sim.ant_orientation == orientation::EAST) {
    next_col = sim.ant_col + 1;
    next_row = sim.ant_row;
  } else if (sim.ant_orientation == orientation::SOUTH) {
    next_row = sim.ant_row + 1;
    next_col = sim.ant_col;
  } else { // west
    next_col = sim.ant_col - 1;
    next_row = sim.ant_row;
  }

  #else

  int next_col;
  if (sim.ant_orientation == orientation::EAST)
    next_col = sim.ant_col + 1;
  else if (sim.ant_orientation == orientation::WEST)
    next_col = sim.ant_col - 1;
  else
    next_col = sim.ant_col;

  int next_row;
  if (sim.ant_orientation == orientation::NORTH)
    next_row = sim.ant_row - 1;
  else if (sim.ant_orientation == orientation::SOUTH)
    next_row = sim.ant_row + 1;
  else
    next_row = sim.ant_row;

  #endif

  if (
    util::in_range_incl_excl(next_col, 0, sim.grid_width) &&
    util::in_range_incl_excl(next_row, 0, sim.grid_height)
  ) [[likely]] {
    sim.ant_col = next_col;
    sim.ant_row = next_row;
    return step_result::SUCCESS;

  } else [[unlikely]] {
    return step_result::FAILED_AT_BOUNDARY;
  }
}

using json_t = nlohmann::json;

// The .what() string of a nlohmann::json exception has the format:
// "[json.exception.x.y] explanation" where x is a class name and y is a positive integer.
//                       ^ This function extracts a string starting from exactly this point.
template <typename Ty>
char const *nlohmann_json_extract_sentence(Ty const &except) {
  auto const from_first_space = std::strchr(except.what(), ' ');
  if (from_first_space == nullptr) {
    throw std::runtime_error("unable to find first space in nlohmann.json exception string");
  } else {
    return from_first_space + 1; // trim off leading space
  }
}

namespace fs = std::filesystem;

void ant::simulation_save(
  simulation const &sim,
  char const *const name,
  fs::path const &dir,
  pgm8::format const fmt
) {
  if (!fs::is_directory(dir)) {
    throw std::runtime_error(
      error("\"%s\" is not a directory", dir.string().c_str())
    );
  }

  std::string name_with_gens = name;
  name_with_gens += "(";
  name_with_gens += std::to_string(sim.generations);
  name_with_gens += ")";

  std::filesystem::path p(dir);
  p /= (name_with_gens + ".json");

  {
    std::ofstream sim_file(p);
    if (!sim_file.is_open()) {
      throw std::runtime_error(
        error("failed to open file \"%s\"", p.string().c_str())
      );
    }

    json_t json, temp;
    json_t::array_t rules_json, save_points_json;

    json["generations"] = sim.generations;
    json["last_step_result"] = ant::step_result::to_string(sim.last_step_res);
    json["grid_width"] = sim.grid_width;
    json["grid_height"] = sim.grid_height;
    json["grid_state"] = (name_with_gens + ".pgm");
    json["ant_col"] = sim.ant_col;
    json["ant_row"] = sim.ant_row;
    json["ant_orientation"] = ant::orientation::to_string(sim.ant_orientation);
    json["save_interval"] = sim.save_interval;

    for (usize shade = 0; shade < sim.rules.size(); ++shade) {
      auto const &rule = sim.rules[shade];
      if (rule.turn_dir != ant::turn_direction::NIL) {
        temp["shade"] = shade;
        temp["replacement"] = rule.replacement_shade;
        temp["turn"] = turn_direction::to_string(rule.turn_dir);
        rules_json.push_back(temp);
        temp.clear();
      }
    }
    json["rules"] = rules_json;

    for (usize i = 0; i < sim.num_save_points; ++i) {
      save_points_json.push_back(sim.save_points[i]);
    }
    std::reverse(save_points_json.begin(), save_points_json.end());
    json["save_points"] = save_points_json;

    sim_file << json.dump(2);
  }

  p.replace_extension(".pgm");
  {
    std::ofstream img_file(p);
    if (!img_file.is_open()) {
      std::stringstream err{};
      err << "failed to open file \"" << p << '"';
      throw std::runtime_error(err.str());
    }

    pgm8::image_properties img_props;
    img_props.set_format(fmt);
    img_props.set_width(static_cast<u16>(sim.grid_width));
    img_props.set_height(static_cast<u16>(sim.grid_height));
    // TODO: don't compute_maxval, deduce maxval from sim.rules instead
    img_props.set_maxval(compute_maxval(sim.grid, sim.grid_width, sim.grid_height));

    pgm8::write(img_file, img_props, sim.grid);
  }
}

template <typename PropTy>
bool try_to_parse_and_set_uint(
  json_t const &json,
  char const *const property,
  PropTy &out,
  uintmax_t const min,
  uintmax_t const max,
  std::function<void (std::string &&)> const &emplace_err
) {
  uintmax_t val;

  try {
    val = json[property].get<uintmax_t>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `%s` -> %s", property, nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (!json[property].is_number_unsigned()) {
    emplace_err(error("invalid `%s` -> not an unsigned integer", property));
    return false;
  }

  if (val < min) {
    emplace_err(error("invalid `%s` -> cannot be < %" PRIuMAX, property, min));
    return false;
  }

  if (val > max) {
    emplace_err(error("invalid `%s` -> cannot be > %" PRIuMAX, property, max));
    return false;
  }

  out = static_cast<PropTy>(val);
  return true;
}

template <typename EnumImplTy>
bool try_to_parse_and_set_enum(
  json_t const &json,
  char const *const property,
  std::vector<
    std::pair<std::string, EnumImplTy>
  > const &text_to_integral_mappings,
  EnumImplTy &out,
  std::function<void (std::string &&)> const &emplace_err
) {
  std::string parsed_text;
  try {
    parsed_text = json[property].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `%s` -> %s", property, nlohmann_json_extract_sentence(except)));
    return false;
  }

  for (auto const &pair : text_to_integral_mappings) {
    std::string const &text = pair.first;
    EnumImplTy const integral = pair.second;
    if (text == parsed_text) {
      out = integral;
      return true;
    }
  }

  std::stringstream allowed_values_stream{};
  for (auto const &pair : text_to_integral_mappings) {
    std::string const &text = pair.first;
    allowed_values_stream << text << '|';
  }
  std::string allowed_values_str = allowed_values_stream.str();
  allowed_values_str.pop_back(); // remove trailing pipe (|)
  emplace_err(error("invalid `%s` -> not one of %s", property, allowed_values_str.c_str()));
  return false;
}

static
bool try_to_parse_and_set_rule(
  std::array<ant::rule, 256> &out,
  json_t const &rule,
  usize const index,
  std::function<void (std::string &&)> const &emplace_err
) {
  if (!rule.is_object()) {
    emplace_err("invalid `rules` -> not an array of objects");
    return false;
  }

  if (rule.count("shade") == 0) {
    emplace_err(error("invalid `rules` -> [%zu].shade not defined", index));
    return false;
  }

  if (rule.count("replacement") == 0) {
    emplace_err(error("invalid `rules` -> [%zu].replacement not defined", index));
    return false;
  }

  if (rule.count("turn") == 0) {
    emplace_err(error("invalid `rules` -> [%zu].turn not defined", index));
    return false;
  }

  u8 shade, replacement;
  ant::turn_direction::type turn_dir;

  try {
    if (!rule["shade"].is_number_unsigned()) {
      emplace_err(error("invalid `rules` -> [%zu].shade is not an unsigned integer", index));
      return false;
    }

    uintmax_t const temp = rule["shade"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      emplace_err(error("invalid `rules` -> [%zu].shade is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    shade = static_cast<u8>(temp);

    if (out[shade].turn_dir != ant::turn_direction::NIL) {
      // rule already set
      emplace_err(error("invalid `rules` -> more than one rule for shade %" PRIu8, UINT8_MAX));
      return false;
    }

  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `rules` -> [%zu].shade %s", index, nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    if (!rule["replacement"].is_number_unsigned()) {
      emplace_err(error("invalid `rules` -> [%zu].replacement is not an unsigned integer", index));
      return false;
    }

    uintmax_t const temp = rule["replacement"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      emplace_err(error("invalid `rules` -> [%zu].replacement is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    replacement = rule["replacement"].get<u8>();

  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `rules` -> [%zu].replacement %s", index, nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    std::string const turn = rule["turn"].get<std::string>();

    if (turn == "L") {
      turn_dir = ant::turn_direction::LEFT;
    } else if (turn == "N") {
      turn_dir = ant::turn_direction::NONE;
    } else if (turn == "R") {
      turn_dir = ant::turn_direction::RIGHT;
    } else {
      emplace_err(error("invalid `rules` -> [%zu].turn is not one of L|N|R", index));
      return false;
    }

  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `rules` -> [%zu].turn %s", index, nlohmann_json_extract_sentence(except)));
    return false;
  }

  out[shade] = { replacement, turn_dir };
  return true;
}

static
bool try_to_parse_and_set_save_point(
  std::array<u64, 7> &out,
  json_t const &save_point,
  usize const index,
  std::function<void (std::string &&)> const &emplace_err
) {
  if (!save_point.is_number_unsigned()) {
    emplace_err(error("invalid `save_points` -> [%zu] is not an unsigned integer", index));
    return false;
  }

  u64 val;

  try {
    val = save_point.get<u64>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `save_points` -> [%zu] %s", index, nlohmann_json_extract_sentence(except)));
    return false;
  }

  out[index] = val;
  return true;
}

static
bool try_to_parse_and_set_rules(
  json_t const &json,
  ant::simulation &sim,
  std::function<void (std::string &&)> const &emplace_err
) {
  json_t::array_t rules;
  try {
    rules = json["rules"].get<json_t::array_t>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `rules` -> %s", nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (rules.size() > 256) {
    emplace_err(error("invalid `rules` -> max 256 allowed, but got " PRIu64, rules.size()));
    return false;
  }

  std::array<ant::rule, 256> parsed_rules{};

  for (usize i = 0; i < rules.size(); ++i) {
    if (!try_to_parse_and_set_rule(parsed_rules, rules[i], i, emplace_err)) {
      return false;
    }
  }

  // validation
  {
    std::array<u16, 256> shade_occurences{};
    usize num_defined_rules = 0;

    for (usize i = 0; i < parsed_rules.size(); ++i) {
      auto const &r = parsed_rules[i];
      bool const is_rule_used = r.turn_dir != ant::turn_direction::NIL;
      if (is_rule_used) {
        ++num_defined_rules;
        ++shade_occurences[i];
        ++shade_occurences[r.replacement_shade];
      }
    }

    if (num_defined_rules < 2) {
      emplace_err("invalid `rules` -> fewer than 2 defined");
      return false;
    }

    // usize num_non_zero_occurences = 0, num_non_two_occurences = 0;
    // for (auto const occurencesOfShade : shade_occurences) {
    //   if (occurencesOfShade != 0) {
    //     ++num_non_zero_occurences;
    //     if (occurencesOfShade != 2)
    //       ++num_non_two_occurences;
    //   }
    // }

    // if (num_non_zero_occurences < 2 || num_non_two_occurences > 0) {
    //   emplace_err("invalid `rules` -> don't form a closed chain");
    //   return false;
    // }
  }

  sim.rules = parsed_rules;
  return true;
}

static
bool try_to_parse_and_set_save_points(
  json_t const &json,
  ant::simulation &sim,
  std::function<void (std::string &&)> const &emplace_err
) {
  json_t::array_t save_points;
  try {
    save_points = json["save_points"].get<json_t::array_t>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `save_points` -> %s", nlohmann_json_extract_sentence(except)));
  }

  usize const num_save_points_specified = save_points.size();
  usize const max_save_points_allowed = sim.save_points.size();

  if (num_save_points_specified > max_save_points_allowed) {
    emplace_err(error("invalid `save_points` -> more than " PRIuMAX " not allowed", max_save_points_allowed));
    return false;
  }

  std::array<u64, 7> parsed_save_points{};

  for (usize i = 0; i < num_save_points_specified; ++i) {
    if (
      !try_to_parse_and_set_save_point(
        parsed_save_points, save_points[i], i, emplace_err
      )
    ) {
      return false;
    }
  }

  // validation
  {
    std::unordered_map<
      u64, // point
      usize // num of occurences
    > save_point_occurences{};

    // count how many times each save_point occurs
    for (usize i = 0; i < num_save_points_specified; ++i)
      ++save_point_occurences[parsed_save_points[i]];

    // check for repeats
    for (auto const [save_point, num_occurences] : save_point_occurences) {
      if (num_occurences > 1) {
        emplace_err(error("save_point `" PRIuFAST64 "` repeated %zu times", save_point, num_occurences));
        return false;
      }
    }

    // check for zeroes
    for (usize i = 0; i < num_save_points_specified; ++i) {
      if (parsed_save_points[i] == 0) {
        emplace_err("save_point cannot be `0`");
        return false;
      }
    }
  }

  sim.save_points = parsed_save_points;
  sim.num_save_points = num_save_points_specified;

  return true;
}

static
bool try_to_parse_and_set_grid_state(
  json_t const &json,
  fs::path const &dir,
  ant::simulation &sim,
  std::function<void (std::string &&)> const &emplace_err,
  std::vector<std::string> const &errors
) {
  std::string grid_state;
  try {
    grid_state = json["grid_state"].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(error("invalid `grid_state` -> %s", nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (grid_state == "") {
    emplace_err("invalid `grid_state` -> cannot be blank");
    return false;
  }

  if (std::regex_match(
    grid_state,
    std::regex("^fill -{1,}[0-9]{1,}$", std::regex_constants::icase))
  ) {
    emplace_err("invalid `grid_state` -> fill value cannot be negative");
    return false;
  }

  if (std::regex_match(
    grid_state,
    std::regex("^fill [0-9]{1,}$", std::regex_constants::icase))
  ) {
    char const *const num_str = grid_state.c_str() + 4;
    uintmax_t const fill_val = strtoumax(num_str, nullptr, 10);

    if (fill_val > UINT8_MAX) {
      emplace_err(error("invalid `grid_state` -> fill value must be <= %" PRIu8, UINT8_MAX));
      return false;
    }

    if (!errors.empty()) {
      return false;
    }

    if (sim.rules[fill_val].turn_dir == ant::turn_direction::NIL) {
      emplace_err("invalid `grid_state` -> fill value has no governing rule");
      return false;
    }

    // only try to allocate and setup grid if there are no errors
    usize const num_pixels = static_cast<usize>(sim.grid_width) * sim.grid_height;
    try {
      sim.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      emplace_err(error("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    std::fill_n(sim.grid, num_pixels, static_cast<u8>(fill_val));
    return true;

  } else {
    fs::path const img_path = dir / grid_state;

    if (!fs::exists(img_path)) {
      emplace_err(error("invalid `grid_state` -> file \"%s\" does not exist", grid_state.c_str()));
      return false;
    }

    std::ifstream file(img_path);
    if (!file.is_open()) {
      emplace_err(error("unable to open file \"%s\"", grid_state.c_str()));
      return false;
    }

    pgm8::image_properties img_props;
    try {
      img_props = pgm8::read_properties(file);
    } catch (std::runtime_error const &except) {
      emplace_err(error("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    usize const num_pixels = img_props.num_pixels();

    try {
      sim.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      emplace_err(error("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    try {
      pgm8::read_pixels(file, img_props, sim.grid);
    } catch (std::runtime_error const &except) {
      emplace_err(error("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    return true;
  }
}

ant::simulation_parse_result_t ant::simulation_parse(
  std::string const &str,
  fs::path const &dir
) {
  ant::simulation sim{};
  std::vector<std::string> errors{};
  std::stringstream err{};

  auto const emplace_err = [&errors](std::string &&err) {
    errors.emplace_back(err);
  };

  json_t json;
  try {
    json = json_t::parse(str);
  } catch (json_t::basic_json::parse_error const &except) {
    emplace_err(error("%s", nlohmann_json_extract_sentence(except)));
    return { sim, errors };
  } catch (...) {
    emplace_err("unexpected error whilst trying to parse simulation");
    return { sim, errors };
  }

  if (!json.is_object()) {
    emplace_err("parsed simulation is not a JSON object");
    return { sim, errors };
  }

  auto const validate_property_set = [&json, &emplace_err, &err](
    char const *const key
  ) {
    usize const occurences = json.count(key);
    if (occurences == 0) {
      emplace_err(error("`%s` not set", key));
      return false;
    } else {
      return true;
    }
  };

  {
    bool good = true;

    good &= validate_property_set("generations");
    good &= validate_property_set("last_step_result");
    good &= validate_property_set("grid_width");
    good &= validate_property_set("grid_height");
    good &= validate_property_set("grid_state");
    good &= validate_property_set("ant_col");
    good &= validate_property_set("ant_row");
    good &= validate_property_set("ant_orientation");
    good &= validate_property_set("rules");
    good &= validate_property_set("save_interval");
    good &= validate_property_set("save_points");

    if (!good) {
      // if any properties not set, stop parsing
      return { sim, errors };
    }
  }

  [[maybe_unused]]
  bool const generations_parse_success = try_to_parse_and_set_uint<u64>(
    json, "generations", sim.generations,
    0, UINT_FAST64_MAX,
    emplace_err
  );

  bool const grid_width_parse_success = try_to_parse_and_set_uint<i32>(
    json, "grid_width", sim.grid_width,
    0, UINT16_MAX,
    emplace_err
  );

  bool const grid_height_parse_success = try_to_parse_and_set_uint<i32>(
    json, "grid_height", sim.grid_height,
    0, UINT16_MAX,
    emplace_err
  );

  bool const ant_col_parse_success = try_to_parse_and_set_uint<i32>(
    json, "ant_col", sim.ant_col,
    0, UINT16_MAX,
    emplace_err
  );

  bool const ant_row_parse_success = try_to_parse_and_set_uint<i32>(
    json, "ant_row", sim.ant_row,
    0, UINT16_MAX,
    emplace_err
  );

  if (
    grid_width_parse_success &&
    (sim.grid_width < 1 || sim.grid_width > UINT16_MAX)
  ) {
    emplace_err(error("invalid `grid_width` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (
    ant_col_parse_success &&
    !util::in_range_incl_excl(sim.ant_col, 0, sim.grid_width)
  ) {
    emplace_err(error("invalid `ant_col` -> not in grid x-axis [0, %d)", sim.grid_width));
  }

  if (
    grid_height_parse_success &&
    (sim.grid_height < 1 || sim.grid_height > UINT16_MAX)
  ) {
    emplace_err(error("invalid `grid_height` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (
    ant_row_parse_success &&
    !util::in_range_incl_excl(sim.ant_row, 0, sim.grid_height)
  ) {
    emplace_err(error("invalid `ant_row` -> not in grid y-axis [0, %d)", sim.grid_width));
  }

  [[maybe_unused]]
  bool const save_interval_parse_success = try_to_parse_and_set_uint<
    u64
  >(
    json, "save_interval", sim.save_interval,
    0, UINT_FAST64_MAX,
    emplace_err
  );

  [[maybe_unused]]
  bool const last_step_res_parse_success = try_to_parse_and_set_enum<
    ant::step_result::type
  >(json, "last_step_result", {
    { "nil", ant::step_result::NIL },
    { "success", ant::step_result::SUCCESS },
    { "failed_at_boundary", ant::step_result::FAILED_AT_BOUNDARY },
  }, sim.last_step_res, emplace_err);

  [[maybe_unused]]
  bool const ant_orientation_parse_success = try_to_parse_and_set_enum<
    ant::orientation::type
  >(json, "ant_orientation", {
    { "north", ant::orientation::NORTH },
    { "east", ant::orientation::EAST },
    { "south", ant::orientation::SOUTH },
    { "west", ant::orientation::WEST },
  }, sim.ant_orientation, emplace_err);

  [[maybe_unused]]
  bool const rules_parse_success = try_to_parse_and_set_rules(
    json, sim, emplace_err
  );

  [[maybe_unused]]
  bool const save_points_parse_success = try_to_parse_and_set_save_points(
    json, sim, emplace_err
  );

  [[maybe_unused]]
  bool const grid_state_parse_success = try_to_parse_and_set_grid_state(
    json, dir, sim, emplace_err, errors
  );

  return { sim, errors };
}

void ant::simulation_run(
  simulation &sim,
  char const *const name,
  u64 const generation_target,
  pgm8::format const img_fmt,
  fs::path const &save_dir
) {
  std::sort(
    sim.save_points.begin(),
    sim.save_points.begin() + sim.num_save_points,
    std::greater<u64>()
  );

  usize num_save_points_left = sim.num_save_points;

  if (sim.save_interval == 0) { // no save interval

    while (true) {
      sim.last_step_res = simulation_step_forward(sim);

      if (sim.last_step_res != step_result::SUCCESS) [[unlikely]] {
        break;
      }

      ++sim.generations;

      if (
        num_save_points_left > 0 &&
        sim.generations == sim.save_points[(num_save_points_left--) - 1]
      ) [[unlikely]] {
        simulation_save(sim, name, save_dir, img_fmt);
      }

      if (sim.generations == generation_target) [[unlikely]] {
        break;
      }
    }
  } else { // with save interval

    while (true) {
      sim.last_step_res = simulation_step_forward(sim);

      if (sim.last_step_res != step_result::SUCCESS) [[unlikely]] {
        break;
      }

      ++sim.generations;

      if (sim.generations % sim.save_interval == 0) [[unlikely]] {
        simulation_save(sim, name, save_dir, img_fmt);
      } else if (
        num_save_points_left > 0 &&
        sim.generations == sim.save_points[num_save_points_left - 1]
      ) [[unlikely]] {
        simulation_save(sim, name, save_dir, img_fmt);
        --num_save_points_left;
      }

      if (sim.generations == generation_target) [[unlikely]] {
        break;
      }
    }
  }
}
