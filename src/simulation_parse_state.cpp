#include <functional>
#include <regex>
#include <sstream>
#include <string>

#include "lib/json.hpp"

#include "primitives.hpp"
#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::errors_t;
using util::make_str;

template <typename PropTy>
b8 try_to_parse_and_set_uint(
  json_t const &json,
  char const *const property,
  PropTy &out,
  uintmax_t const min,
  uintmax_t const max,
  std::function<void(std::string &&)> const &add_err)
{
  uintmax_t val;

  try {
    val = json[property].get<uintmax_t>();
  } catch (json_t::basic_json::type_error const &except) {
    add_err(
      make_str("invalid `%s` -> %s", property, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (!json[property].is_number_unsigned()) {
    add_err(make_str("invalid `%s` -> not an unsigned integer", property));
    return false;
  }

  if (val < min) {
    add_err(make_str("invalid `%s` -> cannot be < %" PRIuMAX, property, min));
    return false;
  }

  if (val > max) {
    add_err(make_str("invalid `%s` -> cannot be > %" PRIuMAX, property, max));
    return false;
  }

  out = static_cast<PropTy>(val);
  return true;
}

template <typename EnumImplTy>
b8 try_to_parse_and_set_enum(
  json_t const &json,
  char const *const property,
  std::vector<std::pair<std::string, EnumImplTy>> const &text_to_integral_mappings,
  EnumImplTy &out,
  std::function<void(std::string &&)> const &add_err)
{
  std::string parsed_text;

  try {
    parsed_text = json[property].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    add_err(
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
  add_err(make_str("invalid `%s` -> not one of %s", property, allowed_values_str.c_str()));
  return false;
}

static
b8 try_to_parse_and_set_rule(
  std::array<simulation::rule, 256> &out,
  json_t const &rule,
  usize const index,
  std::function<void(std::string &&)> const &add_err)
{
  if (!rule.is_object()) {
    add_err("invalid `rules` -> not an array of objects");
    return false;
  }

  if (rule.count("on") == 0) {
    add_err(make_str("invalid `rules` -> [%zu].on not defined", index));
    return false;
  }

  if (rule.count("replace_with") == 0) {
    add_err(make_str("invalid `rules` -> [%zu].replace_with not defined", index));
    return false;
  }

  if (rule.count("turn") == 0) {
    add_err(make_str("invalid `rules` -> [%zu].turn not defined", index));
    return false;
  }

  u8 shade, replacement;
  simulation::turn_direction::value_type turn_dir;

  try {
    if (!rule["on"].is_number_unsigned()) {
      add_err(make_str("invalid `rules` -> [%zu].on is not an unsigned integer", index));
      return false;
    }

    uintmax_t const temp = rule["on"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      add_err(make_str("invalid `rules` -> [%zu].on is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    shade = static_cast<u8>(temp);

    if (out[shade].turn_dir != simulation::turn_direction::NIL) {
      // rule already set
      add_err(make_str("invalid `rules` -> more than one rule for shade %" PRIu8, shade));
      return false;
    }
  } catch (json_t::basic_json::type_error const &except) {
    add_err(make_str("invalid `rules` -> [%zu].on %s", index, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    if (!rule["replace_with"].is_number_unsigned()) {
      add_err(make_str("invalid `rules` -> [%zu].replace_with is not an unsigned integer", index));
      return false;
    }

    uintmax_t const temp = rule["replace_with"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      add_err(make_str("invalid `rules` -> [%zu].replace_with is > %" PRIu8, index, UINT8_MAX));
      return false;
    }

    replacement = rule["replace_with"].get<u8>();
  } catch (json_t::basic_json::type_error const &except) {
    add_err(make_str("invalid `rules` -> [%zu].replace_with %s", index, util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  try {
    std::string const turn = rule["turn"].get<std::string>();

    if (turn.empty()) {
      add_err(make_str("invalid `rules` -> [%zu].turn is empty", index));
      return false;
    }
    if (turn.length() > 1) {
      add_err(make_str("invalid `rules` -> [%zu].turn not recognized", index));
      return false;
    }

    turn_dir = simulation::turn_direction::from_char(turn.front());

  } catch (json_t::basic_json::type_error const &except) {
    add_err(make_str("invalid `rules` -> [%zu].turn %s", index, util::nlohmann_json_extract_sentence(except)));
    return false;
  } catch (std::runtime_error const &) {
    add_err(make_str("invalid `rules` -> [%zu].turn not recognized", index));
    return false;
  }

  out[shade] = { replacement, turn_dir };
  return true;
}

static
b8 try_to_parse_and_set_rules(
  json_t const &json,
  simulation::state &state,
  std::function<void(std::string &&)> const &add_err)
{
  json_t::array_t rules;
  try {
    rules = json["rules"].get<json_t::array_t>();
  } catch (json_t::basic_json::type_error const &except) {
    add_err(make_str("invalid `rules` -> %s", util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (rules.size() > 256) {
    add_err(make_str("invalid `rules` -> max 256 allowed, but got " PRIu64, rules.size()));
    return false;
  }

  std::array<simulation::rule, 256> parsed_rules{};

  for (usize i = 0; i < rules.size(); ++i) {
    if (!try_to_parse_and_set_rule(parsed_rules, rules[i], i, add_err)) {
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
      add_err("invalid `rules` -> fewer than 2 defined");
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
      add_err("invalid `rules` -> don't form a closed chain");
      return false;
    }
  }

  state.rules = parsed_rules;
  return true;
}

static
b8 try_to_parse_and_set_grid_state(
  json_t const &json,
  fs::path const &dir,
  simulation::state &state,
  std::function<void(std::string &&)> const &add_err,
  errors_t const &errors)
{
  std::string grid_state;

  try {
    grid_state = json["grid_state"].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    add_err(
      make_str("invalid `grid_state` -> %s", util::nlohmann_json_extract_sentence(except)));
    return false;
  }

  if (grid_state == "") {
    add_err("invalid `grid_state` -> cannot be blank");
    return false;
  }

  if (std::regex_match(grid_state, std::regex("^fill -{1,}[0-9]{1,}$", std::regex_constants::icase))) {
    add_err("invalid `grid_state` -> fill shade cannot be negative");
    return false;
  }

  if (std::regex_match(grid_state, std::regex("^fill [0-9]{1,}$", std::regex_constants::icase))) {
    char const *const num_str = grid_state.c_str() + 4;
    uintmax_t const fill_val = strtoumax(num_str, nullptr, 10);

    if (fill_val > UINT8_MAX) {
      add_err(make_str("invalid `grid_state` -> fill shade must be <= %" PRIu8, UINT8_MAX));
      return false;
    }

    if (!errors.empty()) {
      return false;
    }

    if (state.rules[fill_val].turn_dir == simulation::turn_direction::NIL) {
      add_err("invalid `grid_state` -> fill shade has no governing rule");
      return false;
    }

    // only try to allocate and setup grid if there are no errors
    usize const num_pixels = static_cast<usize>(state.grid_width) * state.grid_height;
    try {
      state.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      add_err(make_str("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    std::fill_n(state.grid, num_pixels, static_cast<u8>(fill_val));
    return true;

  } else {
    fs::path const img_path = dir / grid_state;

    if (!fs::exists(img_path)) {
      add_err(
        make_str("invalid `grid_state` -> file \"%s\" does not exist", grid_state.c_str()));
      return false;
    }

    std::ifstream file(img_path);
    if (!file.is_open()) {
      add_err(make_str("unable to open file \"%s\"", grid_state.c_str()));
      return false;
    }

    pgm8::image_properties img_props;
    try {
      img_props = pgm8::read_properties(file);
    } catch (std::runtime_error const &except) {
      add_err(make_str("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    usize const num_pixels = img_props.num_pixels();
    {
      usize const num_pixels_expected = static_cast<usize>(state.grid_width) * state.grid_height;
      if (num_pixels != num_pixels_expected) {
        if (state.grid_width != img_props.get_width()) {
          add_err(make_str("dimension mismatch, image width (%zu) does not correspond to grid width (%zu)",
            img_props.get_width(), state.grid_width));
        }
        if (state.grid_height != img_props.get_height()){
          add_err(make_str("dimension mismatch, image height (%zu) does not correspond to grid height (%zu)",
            img_props.get_height(), state.grid_height));
        }
        return false;
      }
    }

    try {
      state.grid = new u8[num_pixels];
    } catch (std::bad_alloc const &) {
      add_err(make_str("failed to allocate %zu bytes for grid", num_pixels * sizeof(u8)));
      return false;
    }

    try {
      pgm8::read_pixels(file, img_props, state.grid);
    } catch (std::runtime_error const &except) {
      add_err(make_str("failed to read file \"%s\" - %s", grid_state.c_str(), except.what()));
      return false;
    }

    return true;
  }
}

std::variant<simulation::state, errors_t> simulation::parse_state(
  std::string const &str,
  fs::path const &dir)
{
  simulation::state state{};
  errors_t errors{};
  std::variant<simulation::state, errors_t> retval;

  auto const add_err = [&errors](std::string &&err) {
    errors.emplace_back(err);
  };

  json_t json;
  try {
    json = json_t::parse(str);
  } catch (json_t::basic_json::parse_error const &except) {
    add_err(make_str("%s", util::nlohmann_json_extract_sentence(except)));
    return errors;
  } catch (...) {
    add_err("unexpected error whilst trying to parse simulation");
    return errors;
  }

  if (!json.is_object()) {
    add_err("parsed simulation is not a JSON object");
    return errors;
  }

  {
    auto const validate_property_set = [&json, &add_err](char const *const key) -> i8 {
      usize const num_occurences = json.count(key);
      if (num_occurences == 0) {
        add_err(make_str("`%s` not set", key));
        return 0;
      } else {
        return 1;
      }
    };

    i8 good = true;

    good &= validate_property_set("generation");
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
      return errors;
    }
  }

  [[maybe_unused]] b8 const generation_parse_success = try_to_parse_and_set_uint<u64>(
    json, "generation", state.generation, 0, UINT_FAST64_MAX, add_err
  );

  b8 const grid_width_parse_success = try_to_parse_and_set_uint<i32>(
    json, "grid_width", state.grid_width, 0, UINT16_MAX, add_err
  );

  b8 const grid_height_parse_success = try_to_parse_and_set_uint<i32>(
    json, "grid_height", state.grid_height, 0, UINT16_MAX, add_err
  );

  b8 const ant_col_parse_success = try_to_parse_and_set_uint<i32>(
  json, "ant_col", state.ant_col, 0, UINT16_MAX, add_err
  );

  b8 const ant_row_parse_success = try_to_parse_and_set_uint<i32>(
    json, "ant_row", state.ant_row, 0, UINT16_MAX, add_err
  );

  if (grid_width_parse_success && (state.grid_width < 1 || state.grid_width > UINT16_MAX)) {
    add_err(make_str("invalid `grid_width` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (ant_col_parse_success && !util::in_range_incl_excl(state.ant_col, 0, state.grid_width)) {
    add_err(make_str("invalid `ant_col` -> not in grid x-axis [0, %d)", state.grid_width));
  }

  if (grid_height_parse_success && (state.grid_height < 1 || state.grid_height > UINT16_MAX)) {
    add_err(make_str("invalid `grid_height` -> not in range [1, %" PRIu16 "]", UINT16_MAX));
  } else if (ant_row_parse_success && !util::in_range_incl_excl(state.ant_row, 0, state.grid_height)) {
    add_err(make_str("invalid `ant_row` -> not in grid y-axis [0, %d)", state.grid_width));
  }

  [[maybe_unused]] b8 const last_step_res_parse_success = try_to_parse_and_set_enum<
    simulation::step_result::value_type
  >(
    json,
    "last_step_result",
    {
      { simulation::step_result::to_string(simulation::step_result::NIL), simulation::step_result::NIL },
      { simulation::step_result::to_string(simulation::step_result::SUCCESS), simulation::step_result::SUCCESS },
      { simulation::step_result::to_string(simulation::step_result::HIT_EDGE), simulation::step_result::HIT_EDGE },
    },
    state.last_step_res,
    add_err
  );

  [[maybe_unused]] b8 const ant_orientation_parse_success = try_to_parse_and_set_enum<
    simulation::orientation::value_type
  >(
    json,
    "ant_orientation",
    {
      { simulation::orientation::to_string(simulation::orientation::NORTH), simulation::orientation::NORTH },
      { simulation::orientation::to_string(simulation::orientation::EAST), simulation::orientation::EAST },
      { simulation::orientation::to_string(simulation::orientation::SOUTH), simulation::orientation::SOUTH },
      { simulation::orientation::to_string(simulation::orientation::WEST), simulation::orientation::WEST },
    },
    state.ant_orientation,
    add_err
  );

  [[maybe_unused]] b8 const rules_parse_success = try_to_parse_and_set_rules(
    json, state, add_err
  );

  [[maybe_unused]] b8 const grid_state_parse_success = try_to_parse_and_set_grid_state(
    json, dir, state, add_err, errors
  );

  if (errors.empty()) {
    return state;
  } else {
    return errors;
  }
}
