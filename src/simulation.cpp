#include "simulation.hpp"

#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "json.hpp"
#include "util.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::make_str;
using util::strvec_t;

static u8
deduce_maxval(std::array<simulation::rule, 256> rules)
{
  u8 maxval = 0;

  for (usize i = 1; i < rules.size(); ++i) {
    if (rules[i].turn_dir != simulation::turn_direction::NIL)
      maxval = static_cast<u8>(i);
  }

  return maxval;
}

char const *
simulation::orientation::to_string(i8 const orient)
{
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

char const *
simulation::turn_direction::to_string(i8 const turn_dir)
{
  switch (turn_dir) {
    case turn_direction::NIL:
      return "nil";
    case turn_direction::LEFT:
      return "L";
    case turn_direction::NO_CHANGE:
      return "N";
    case turn_direction::RIGHT:
      return "R";

    default:
      throw std::runtime_error("turn_direction::to_string failed - bad value");
  }
}

char const *
simulation::step_result::to_string(step_result::value_type const step_res)
{
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

b8
simulation::rule::operator!=(simulation::rule const &other) const noexcept
{
  return this->replacement_shade != other.replacement_shade || this->turn_dir != other.turn_dir;
}

namespace simulation {
  // define ostream insertion operator for the rule struct, for ntest
  std::ostream &
  operator<<(std::ostream &os, simulation::rule const &rule)
  {
    os << "(r=" << std::to_string(rule.replacement_shade)
       << ", d=" << simulation::turn_direction::to_string(rule.turn_dir) << ')';
    return os;
  }
} // namespace simulation

simulation::step_result::value_type
simulation::attempt_step_forward(simulation::configuration &cfg)
{
  usize const curr_cell_idx = (cfg.ant_row * cfg.grid_width) + cfg.ant_col;
  u8 const curr_cell_shade = cfg.grid[curr_cell_idx];
  auto const &curr_cell_rule = cfg.rules[curr_cell_shade];

  // turn
  cfg.ant_orientation = cfg.ant_orientation + curr_cell_rule.turn_dir;
  if (cfg.ant_orientation == orientation::OVERFLOW_COUNTER_CLOCKWISE)
    cfg.ant_orientation = orientation::WEST;
  else if (cfg.ant_orientation == orientation::OVERFLOW_CLOCKWISE)
    cfg.ant_orientation = orientation::NORTH;

  // update current cell shade
  cfg.grid[curr_cell_idx] = curr_cell_rule.replacement_shade;

#if 1

  i32 next_col, next_row;
  if (cfg.ant_orientation == orientation::NORTH) {
    next_col = cfg.ant_col;
    next_row = cfg.ant_row - 1;
  } else if (cfg.ant_orientation == orientation::EAST) {
    next_col = cfg.ant_col + 1;
    next_row = cfg.ant_row;
  } else if (cfg.ant_orientation == orientation::SOUTH) {
    next_row = cfg.ant_row + 1;
    next_col = cfg.ant_col;
  } else { // west
    next_col = cfg.ant_col - 1;
    next_row = cfg.ant_row;
  }

#else

  i32 next_col;
  if (cfg.ant_orientation == orientation::EAST)
    next_col = cfg.ant_col + 1;
  else if (cfg.ant_orientation == orientation::WEST)
    next_col = cfg.ant_col - 1;
  else
    next_col = cfg.ant_col;

  int next_row;
  if (cfg.ant_orientation == orientation::NORTH)
    next_row = cfg.ant_row - 1;
  else if (cfg.ant_orientation == orientation::SOUTH)
    next_row = cfg.ant_row + 1;
  else
    next_row = cfg.ant_row;

#endif

  if (
    util::in_range_incl_excl(next_col, 0, cfg.grid_width) &&
    util::in_range_incl_excl(next_row, 0, cfg.grid_height)) [[likely]] {
    cfg.ant_col = next_col;
    cfg.ant_row = next_row;
    return step_result::SUCCESS;
  } else [[unlikely]] {
    return step_result::FAILED_AT_BOUNDARY;
  }

} // ant::simulation_step_forward

void
simulation::save_config(
  simulation::configuration const &cfg,
  const char *const name,
  fs::path const &dir,
  pgm8::format const fmt)
{
  if (!fs::is_directory(dir)) {
    throw std::runtime_error(make_str("\"%s\" is not a directory", dir.string().c_str()));
  }

  std::string const name_with_gen = make_str("%s(%zu).json", name, cfg.generation);

  std::filesystem::path p(dir);
  p /= name_with_gen;

  {
    std::ofstream cfg_file(p);
    if (!cfg_file.is_open())
      throw std::runtime_error(make_str("failed to open file \"%s\"", p.string().c_str()));

    json_t json, temp;
    json_t::array_t rules_json, save_points_json;

    json["generation"] = cfg.generation;
    json["last_step_result"] = simulation::step_result::to_string(cfg.last_step_res);
    json["grid_width"] = cfg.grid_width;
    json["grid_height"] = cfg.grid_height;
    json["grid_state"] = (name_with_gen + ".pgm");
    json["ant_col"] = cfg.ant_col;
    json["ant_row"] = cfg.ant_row;
    json["ant_orientation"] = simulation::orientation::to_string(cfg.ant_orientation);

    for (usize shade = 0; shade < cfg.rules.size(); ++shade) {
      auto const &rule = cfg.rules[shade];
      if (rule.turn_dir != simulation::turn_direction::NIL) {
        temp["shade"] = shade;
        temp["replacement"] = rule.replacement_shade;
        temp["turn"] = turn_direction::to_string(rule.turn_dir);
        rules_json.push_back(temp);
        temp.clear();
      }
    }
    json["rules"] = rules_json;

    cfg_file << json.dump(2);
  }

  p.replace_extension(".pgm");
  {
    std::ofstream img_file(p);
    if (!img_file.is_open())
      throw std::runtime_error(make_str("failed to open file \"%s\"", p.string().c_str()));

    pgm8::image_properties img_props;
    img_props.set_format(fmt);
    img_props.set_width(static_cast<u16>(cfg.grid_width));
    img_props.set_height(static_cast<u16>(cfg.grid_height));
    img_props.set_maxval(deduce_maxval(cfg.rules));

    pgm8::write(img_file, img_props, cfg.grid);
  }
}

template <typename PropTy>
b8
try_to_parse_and_set_uint(
  json_t const &json,
  char const *const property,
  PropTy &out,
  uintmax_t const min,
  uintmax_t const max,
  std::function<void(std::string &&)> const &emplace_err)
{
  uintmax_t val;

  try {
    val = json[property].get<uintmax_t>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(
      make_str("invalid `%s` -> %s", property, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (!json[property].is_number_unsigned()) {
    emplace_err(make_str("invalid `%s` -> not an unsigned integer", property));
    return false;
  }

  if (val < min) {
    emplace_err(make_str("invalid `%s` -> cannot be < %" PRIuMAX, property, min));
    return false;
  }

  if (val > max) {
    emplace_err(make_str("invalid `%s` -> cannot be > %" PRIuMAX, property, max));
    return false;
  }

  out = static_cast<PropTy>(val);
  return true;
}

template <typename EnumImplTy>
b8
try_to_parse_and_set_enum(
  json_t const &json,
  char const *const property,
  std::vector<std::pair<std::string, EnumImplTy>> const &text_to_integral_mappings,
  EnumImplTy &out,
  std::function<void(std::string &&)> const &emplace_err)
{
  std::string parsed_text;
  try {
    parsed_text = json[property].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(
      make_str("invalid `%s` -> %s", property, util::nlohmann_json_extract_sentence(except)));
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
  allowed_values_str.pop_back(); // remove trailing pipe
  emplace_err(make_str("invalid `%s` -> not one of %s", property, allowed_values_str.c_str()));
  return false;
}

static b8
try_to_parse_and_set_rule(
  std::array<simulation::rule, 256> &out,
  json_t const &rule,
  usize const index,
  std::function<void(std::string &&)> const &emplace_err)
{
  if (!rule.is_object()) {
    emplace_err("invalid `rules` -> not an array of objects");
    return false;
  }

  if (rule.count("shade") == 0) {
    emplace_err(make_str("invalid `rules` -> [%zu].shade not defined", index));
    return false;
  }

  if (rule.count("replacement") == 0) {
    emplace_err(make_str("invalid `rules` -> [%zu].replacement not defined", index));
    return false;
  }

  if (rule.count("turn") == 0) {
    emplace_err(make_str("invalid `rules` -> [%zu].turn not defined", index));
    return false;
  }

  u8 shade, replacement;
  simulation::turn_direction::value_type turn_dir;

  try {
    if (!rule["shade"].is_number_unsigned()) {
      emplace_err(make_str("invalid `rules` -> [%zu].shade is not an unsigned integer", index));
      return false;
    }

    uintmax_t const temp = rule["shade"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      emplace_err(make_str("invalid `rules` -> [%zu].shade is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    shade = static_cast<u8>(temp);

    if (out[shade].turn_dir != simulation::turn_direction::NIL) {
      // rule already set
      emplace_err(make_str("invalid `rules` -> more than one rule for shade %" PRIu8, UINT8_MAX));
      return false;
    }
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(make_str(
      "invalid `rules` -> [%zu].shade %s", index, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    if (!rule["replacement"].is_number_unsigned()) {
      emplace_err(make_str(
        "invalid `rules` -> [%zu].replacement is not an unsigned "
        "integer",
        index));
      return false;
    }

    uintmax_t const temp = rule["replacement"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      emplace_err(make_str("invalid `rules` -> [%zu].replacement is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    replacement = rule["replacement"].get<u8>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(make_str(
      "invalid `rules` -> [%zu].replacement %s",
      index,
      util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    std::string const turn = rule["turn"].get<std::string>();

    if (turn == "L") {
      turn_dir = simulation::turn_direction::LEFT;
    } else if (turn == "N") {
      turn_dir = simulation::turn_direction::NO_CHANGE;
    } else if (turn == "R") {
      turn_dir = simulation::turn_direction::RIGHT;
    } else {
      emplace_err(make_str("invalid `rules` -> [%zu].turn is not one of L|N|R", index));
      return false;
    }
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(make_str(
      "invalid `rules` -> [%zu].turn %s", index, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  out[shade] = { replacement, turn_dir };
  return true;
}

static b8
try_to_parse_and_set_rules(
  json_t const &json,
  simulation::configuration &cfg,
  std::function<void(std::string &&)> const &emplace_err)
{
  json_t::array_t rules;
  try {
    rules = json["rules"].get<json_t::array_t>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(make_str("invalid `rules` -> %s", util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (rules.size() > 256) {
    emplace_err(make_str("invalid `rules` -> max 256 allowed, but got " PRIu64, rules.size()));
    return false;
  }

  std::array<simulation::rule, 256> parsed_rules{};

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
      b8 const is_rule_used = r.turn_dir != simulation::turn_direction::NIL;
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

    usize num_non_zero_occurences = 0, num_non_two_occurences = 0;
    for (auto const occurencesOfShade : shade_occurences) {
      if (occurencesOfShade != 0) {
        ++num_non_zero_occurences;
        if (occurencesOfShade != 2)
          ++num_non_two_occurences;
      }
    }

    if (num_non_zero_occurences < 2 || num_non_two_occurences > 0) {
      emplace_err("invalid `rules` -> don't form a closed chain");
      return false;
    }
  }

  cfg.rules = parsed_rules;
  return true;
}

// static
// b8 try_to_parse_and_set_save_points(
//   json_t const &json,
//   ant::simulation &sim,
//   std::function<void (std::string &&)> const &emplace_err
// ) {
//   json_t::array_t save_points;
//   try {
//     save_points = json["save_points"].get<json_t::array_t>();
//   } catch (json_t::basic_json::type_error const &except) {
//     emplace_err(make_str("invalid `save_points` -> %s",
//     util::nlohmann_json_extract_sentence(except)));
//   }

//   usize const num_save_points_specified = save_points.size();
//   usize const max_save_points_allowed = cfg.save_points.size();

//   if (num_save_points_specified > max_save_points_allowed) {
//     emplace_err(make_str("invalid `save_points` -> more than " PRIuMAX " not
//     allowed", max_save_points_allowed)); return false;
//   }

//   std::array<u64, 7> parsed_save_points{};

//   for (usize i = 0; i < num_save_points_specified; ++i) {
//     if (
//       !try_to_parse_and_set_save_point(
//         parsed_save_points, save_points[i], i, emplace_err
//       )
//     ) {
//       return false;
//     }
//   }

//   // validation
//   {
//     std::unordered_map<
//       u64, // point
//       usize // num of occurences
//     > save_point_occurences{};

//     // count how many times each save_point occurs
//     for (usize i = 0; i < num_save_points_specified; ++i)
//       ++save_point_occurences[parsed_save_points[i]];

//     // check for repeats
//     for (auto const [save_point, num_occurences] : save_point_occurences) {
//       if (num_occurences > 1) {
//         emplace_err(make_str("save_point `" PRIuFAST64 "` repeated %zu
//         times", save_point, num_occurences)); return false;
//       }
//     }

//     // check for zeroes
//     for (usize i = 0; i < num_save_points_specified; ++i) {
//       if (parsed_save_points[i] == 0) {
//         emplace_err("save_point cannot be `0`");
//         return false;
//       }
//     }
//   }

//   cfg.save_points = parsed_save_points;
//   cfg.num_save_points = num_save_points_specified;

//   return true;
// }

static b8
try_to_parse_and_set_grid_state(
  json_t const &json,
  fs::path const &dir,
  simulation::configuration &cfg,
  std::function<void(std::string &&)> const &emplace_err,
  strvec_t const &errors)
{
  std::string grid_state;

  try {
    grid_state = json["grid_state"].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    emplace_err(
      make_str("invalid `grid_state` -> %s", util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (grid_state == "") {
    emplace_err("invalid `grid_state` -> cannot be blank");
    return false;
  }

  if (std::regex_match(
        grid_state, std::regex("^fill -{1,}[0-9]{1,}$", std::regex_constants::icase))) {
    emplace_err("invalid `grid_state` -> fill value cannot be negative");
    return false;
  }

  if (std::regex_match(grid_state, std::regex("^fill [0-9]{1,}$", std::regex_constants::icase))) {
    char const *const num_str = grid_state.c_str() + 4;
    uintmax_t const fill_val = strtoumax(num_str, nullptr, 10);

    if (fill_val > UINT8_MAX) {
      emplace_err(make_str("invalid `grid_state` -> fill value must be <= %" PRIu8, UINT8_MAX));
      return false;
    }

    if (!errors.empty()) {
      return false;
    }

    if (cfg.rules[fill_val].turn_dir == simulation::turn_direction::NIL) {
      emplace_err("invalid `grid_state` -> fill value has no governing rule");
      return false;
    }

    // only try to allocate and setup grid if there are no errors
    usize const num_pixels = static_cast<usize>(cfg.grid_width) * cfg.grid_height;
    try {
      cfg.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      emplace_err(make_str("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    std::fill_n(cfg.grid, num_pixels, static_cast<u8>(fill_val));
    return true;

  } else {
    fs::path const img_path = dir / grid_state;

    if (!fs::exists(img_path)) {
      emplace_err(
        make_str("invalid `grid_state` -> file \"%s\" does not exist", grid_state.c_str()));
      return false;
    }

    std::ifstream file(img_path);
    if (!file.is_open()) {
      emplace_err(make_str("unable to open file \"%s\"", grid_state.c_str()));
      return false;
    }

    pgm8::image_properties img_props;
    try {
      img_props = pgm8::read_properties(file);
    } catch (std::runtime_error const &except) {
      emplace_err(make_str("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    usize const num_pixels = img_props.num_pixels();

    try {
      cfg.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      emplace_err(make_str("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    try {
      pgm8::read_pixels(file, img_props, cfg.grid);
    } catch (std::runtime_error const &except) {
      emplace_err(make_str("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    return true;
  }
}

simulation::configuration
simulation::parse_config(std::string const &str, fs::path const &dir, strvec_t &errors)
{
  simulation::configuration cfg{};
  std::stringstream err{};

  auto const emplace_err = [&errors](std::string &&err) { errors.emplace_back(err); };

  json_t json;
  try {
    json = json_t::parse(str);
  } catch (json_t::basic_json::parse_error const &except) {
    emplace_err(make_str("%s", util::nlohmann_json_extract_sentence(except)));
    return cfg;
  } catch (...) {
    emplace_err("unexpected error whilst trying to parse simulation");
    return cfg;
  }

  if (!json.is_object()) {
    emplace_err("parsed simulation is not a JSON object");
    return cfg;
  }

  {
    auto const validate_property_set = [&json, &emplace_err, &err](char const *const key) -> i8 {
      usize const num_occurences = json.count(key);
      if (num_occurences == 0) {
        emplace_err(make_str("`%s` not set", key));
        return 0;
      } else {
        return 1;
      }
    };

    i8 good = true;

    good &= validate_property_set("generations");
    good &= validate_property_set("last_step_result");
    good &= validate_property_set("grid_width");
    good &= validate_property_set("grid_height");
    good &= validate_property_set("grid_state");
    good &= validate_property_set("ant_col");
    good &= validate_property_set("ant_row");
    good &= validate_property_set("ant_orientation");
    good &= validate_property_set("rules");

    if (!good) {
      // if any properties not set, stop parsing
      return cfg;
    }
  }

  [[maybe_unused]] b8 const generation_parse_success = try_to_parse_and_set_uint<u64>(
    json, "generations", cfg.generation, 0, UINT_FAST64_MAX, emplace_err);

  b8 const grid_width_parse_success =
    try_to_parse_and_set_uint<i32>(json, "grid_width", cfg.grid_width, 0, UINT16_MAX, emplace_err);

  b8 const grid_height_parse_success = try_to_parse_and_set_uint<i32>(
    json, "grid_height", cfg.grid_height, 0, UINT16_MAX, emplace_err);

  b8 const ant_col_parse_success =
    try_to_parse_and_set_uint<i32>(json, "ant_col", cfg.ant_col, 0, UINT16_MAX, emplace_err);

  b8 const ant_row_parse_success =
    try_to_parse_and_set_uint<i32>(json, "ant_row", cfg.ant_row, 0, UINT16_MAX, emplace_err);

  if (grid_width_parse_success && (cfg.grid_width < 1 || cfg.grid_width > UINT16_MAX)) {
    emplace_err(make_str("invalid `grid_width` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (ant_col_parse_success && !util::in_range_incl_excl(cfg.ant_col, 0, cfg.grid_width)) {
    emplace_err(make_str("invalid `ant_col` -> not in grid x-axis [0, %d)", cfg.grid_width));
  }

  if (grid_height_parse_success && (cfg.grid_height < 1 || cfg.grid_height > UINT16_MAX)) {
    emplace_err(make_str("invalid `grid_height` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (ant_row_parse_success && !util::in_range_incl_excl(cfg.ant_row, 0, cfg.grid_height)) {
    emplace_err(make_str("invalid `ant_row` -> not in grid y-axis [0, %d)", cfg.grid_width));
  }

  [[maybe_unused]] b8 const last_step_res_parse_success =
    try_to_parse_and_set_enum<simulation::step_result::value_type>(
      json,
      "last_step_result",
      {
        { "nil", simulation::step_result::NIL },
        { "success", simulation::step_result::SUCCESS },
        { "failed_at_boundary", simulation::step_result::FAILED_AT_BOUNDARY },
      },
      cfg.last_step_res,
      emplace_err);

  [[maybe_unused]] b8 const ant_orientation_parse_success =
    try_to_parse_and_set_enum<simulation::orientation::value_type>(
      json,
      "ant_orientation",
      {
        { "north", simulation::orientation::NORTH },
        { "east", simulation::orientation::EAST },
        { "south", simulation::orientation::SOUTH },
        { "west", simulation::orientation::WEST },
      },
      cfg.ant_orientation,
      emplace_err);

  [[maybe_unused]] b8 const rules_parse_success =
    try_to_parse_and_set_rules(json, cfg, emplace_err);

  [[maybe_unused]] b8 const grid_state_parse_success =
    try_to_parse_and_set_grid_state(json, dir, cfg, emplace_err, errors);

  return cfg;
}

namespace helpers {

  /* Finds the index of the smallest value in an array. */
  template <typename Ty>
  usize
  idx_of_smallest(Ty const *const values, usize const num_values)
  {
    assert(num_values > 0);
    assert(values != nullptr);

    usize min_idx = 0;
    for (usize i = 1; i < num_values; ++i) {
      if (values[i] < values[min_idx]) {
        min_idx = i;
      }
    }

    return min_idx;

  } // idx_of_smallest

  /* Removes duplicate elements from a sorted vector. */
  template <typename ElemTy>
  std::vector<ElemTy>
  remove_duplicates_sorted(std::vector<ElemTy> const &input)
  {
    if (input.size() < 2) {
      return input;
    }

    std::vector<ElemTy> output(input.size());

    // check the first element individually
    if (input[0] != input[1]) {
      output.push_back(input[0]);
    }

    for (usize i = 1; i < input.size() - 1; ++i) {
      if (input[i] != input[i - 1] && input[i] != input[i + 1]) {
        output.push_back(input[i]);
      }
    }

    // check the last item individually
    if (input[input.size() - 1] != input[input.size() - 2]) {
      output.push_back(input[input.size() - 1]);
    }

    return output;

  } // remove_duplicates

} // namespace helpers

/*
  Runs a simulation (`sim`) until either the `generation_target` is reached
  or the ant tries to step off the grid.

  `save_points` is a list of points at which to emit a save.

  `save_interval` specifies an interval to emit saves, 0 indicates no interval.

  `save_dir` is where any saves (.json and image files) are emitted.

  If `save_final_cfg` is true, the final state of the simulation is saved,
  regardless of `save_points` and `save_interval`. If the final state happens
  to be saved because of a save_point or the `save_interval`, a duplicate save
  is not made.
*/
void
simulation::run(
  configuration &cfg,
  std::string const name,
  u64 const generation_target,
  std::vector<u64> save_points,
  u64 const save_interval,
  pgm8::format const img_fmt,
  std::filesystem::path const &save_dir,
  b8 save_final_cfg)
{
  // sort save_points in descending order, so we can pop them off the back as we complete them
  std::sort(save_points.begin(), save_points.end(), std::greater<u64>());
  save_points = helpers::remove_duplicates_sorted(save_points);

  u64 last_saved_gen = UINT64_MAX;

  for (;;) {
    // the most generations we can perform before we overflow cfg.generation
    u64 const max_dist = UINT64_MAX - cfg.generation;

    u64 const dist_to_next_save_interval =
      save_interval == 0 ? max_dist : save_interval - (cfg.generation % save_interval);

    u64 const dist_to_next_save_point =
      save_points.empty() ? max_dist : save_points.back() - cfg.generation;

    u64 const dist_to_gen_target = generation_target - cfg.generation;

    std::array<u64, 3> const distances{
      dist_to_next_save_interval,
      dist_to_next_save_point,
      dist_to_gen_target,
    };

    enum stop_reason : usize {
      SAVE_INTERVAL = 0,
      SAVE_POINT,
      GENERATION_TARGET,
    };

    struct stop {
      u64 distance;
      stop_reason reason;
    };

    stop next_stop;
    usize const idx_of_smallest_dist = helpers::idx_of_smallest(distances.data(), distances.size());
    next_stop.distance = distances[idx_of_smallest_dist];
    next_stop.reason = static_cast<stop_reason>(idx_of_smallest_dist);

    if (next_stop.reason == stop_reason::SAVE_POINT) {
      save_points.pop_back();
    }

    for (usize i = 0; i < next_stop.distance; ++i) {
      cfg.last_step_res = simulation::attempt_step_forward(cfg);
      if (cfg.last_step_res == simulation::step_result::SUCCESS) [[likely]] {
        ++cfg.generation;
      } else [[unlikely]] {
        goto done;
      }
    }

    if (next_stop.reason != stop_reason::GENERATION_TARGET) {
      simulation::save_config(cfg, name.c_str(), save_dir, img_fmt);
      last_saved_gen = cfg.generation;
    } else {
      goto done;
    }
  }
done:

  b8 const final_cfg_already_saved = last_saved_gen == cfg.generation;
  if (save_final_cfg && !final_cfg_already_saved) {
    simulation::save_config(cfg, name.c_str(), save_dir, img_fmt);
  }
}

b8
simulation::configuration::can_step_forward(u64 const generation_target) const noexcept
{
  return (last_step_res <= simulation::step_result::SUCCESS) && (generation < generation_target);
}
