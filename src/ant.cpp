#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <regex>
#include <unordered_map>

#include "ant.hpp"
#include "arr2d.hpp"
#include "cstr.hpp"
#include "json.hpp"
#include "util.hpp"

char const *ant::orientation::to_string(int8_t const orient) {
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

char const *ant::turn_direction::to_string(int8_t const turn_dir) {
  switch (turn_dir) {
    case turn_direction::NIL:
      return "nil";
    case turn_direction::LEFT:
      return "left";
    case turn_direction::NONE:
      return "none";
    case turn_direction::RIGHT:
      return "right";

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
  size_t const curr_cell_idx = (sim.ant_row * sim.grid_width) + sim.ant_col;
  uint8_t const curr_cell_shade = sim.grid[curr_cell_idx];
  auto const &curr_cell_rule = sim.rules[curr_cell_shade];

  // turn
  sim.ant_orientation = sim.ant_orientation + curr_cell_rule.turn_dir;
  if (sim.ant_orientation == orientation::OVERFLOW_COUNTER_CLOCKWISE)
    sim.ant_orientation = orientation::WEST;
  else if (sim.ant_orientation == orientation::OVERFLOW_CLOCKWISE)
    sim.ant_orientation = orientation::NORTH;

  // update current cell shade
  sim.grid[curr_cell_idx] = curr_cell_rule.replacement_shade;

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

void ant::simulation_save(simulation const &sim, char const *name, pgm8::format const fmt) {
  char sim_file_pathname[256];
  std::snprintf(
    sim_file_pathname, util::lengthof(sim_file_pathname), "%s.sim", name
  );

  char img_pathname[256];
  std::snprintf(
    sim_file_pathname, util::lengthof(sim_file_pathname), "%s.pgm", name
  );

  {
    std::ofstream sim_file(sim_file_pathname);

    char const *const delim = ";\n", *const indent = "  ";

    sim_file
      << "name = " << name << delim
      << "iterations_completed = " << sim.generations << delim
      << "last_step_result = " << step_result::to_string(sim.last_step_res) << delim
      << "grid_width = " << sim.grid_width << delim
      << "grid_height = " << sim.grid_height << delim
      << "grid_state = file \"" << img_pathname << '"' << delim
      << "ant_col = " << sim.ant_col << delim
      << "ant_row = " << sim.ant_row << delim
      << "ant_orientation = " << orientation::to_string(sim.ant_orientation) << delim
      << "save_interval = " << sim.save_interval << delim
    ;

    sim_file << "rules = [\n";
    for (size_t shade = 0; shade < sim.rules.size(); ++shade) {
      auto const &rule = sim.rules[shade];
      if (rule.turn_dir != turn_direction::NIL)
        sim_file
          << indent << '{' << shade << ','
          << static_cast<int>(rule.replacement_shade) << ','
          << turn_direction::to_string(rule.turn_dir) << "},\n";
    }
    sim_file << ']' << delim;

    sim_file << "save_points = [\n";
    for (size_t i = 0; i < sim.save_points.size(); ++i)
      sim_file << indent << sim.save_points[i] << ",\n";
    sim_file << ']' << delim;
  }

  {
    std::ofstream file(img_pathname);
    if (!file.is_open()) {
      // error
      return;
    }

    pgm8::image_properties img_props;
    img_props.set_format(fmt);
    img_props.set_width(static_cast<uint16_t>(sim.grid_width));
    img_props.set_height(static_cast<uint16_t>(sim.grid_height));
    img_props.set_maxval(arr2d::max(sim.grid, sim.grid_width, sim.grid_height));

    pgm8::write(file, img_props, sim.grid);
  }
}

template <typename PropTy>
bool try_to_parse_and_set_uint(
  json_t const &json,
  char const *const property,
  PropTy &out,
  uintmax_t const min,
  uintmax_t const max,
  std::stringstream &err,
  std::function<void (std::stringstream &)> const &emplace_err
) {
  uintmax_t val;

  try {
    val = json[property].get<uintmax_t>();
  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `" << property << "` -> " << nlohmann_json_extract_sentence(except);
    emplace_err(err);
    return false;
  }

  if (!json[property].is_number_unsigned()) {
    err << "invalid `" << property << "` -> not an unsigned integer";
    emplace_err(err);
    return false;
  }

  if (val < min) {
    err << "invalid `" << property << "` -> cannot be < " << min;
    emplace_err(err);
    return false;
  }

  if (val > max) {
    err << "invalid `" << property << "` -> cannot be > " << max;
    emplace_err(err);
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
  std::stringstream &err,
  std::function<void (std::stringstream &)> const &emplace_err
) {
  std::string parsed_text;
  try {
    parsed_text = json[property].get<std::string>();
  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `" << property << "` -> " << nlohmann_json_extract_sentence(except);
    emplace_err(err);
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
  err << "invalid `" << property << "` -> not one of " << allowed_values_str;
  emplace_err(err);
  return false;
}

static
bool try_to_parse_and_set_rule(
  std::array<ant::rule, 256> &out,
  json_t const &rule,
  size_t const index,
  std::stringstream &err,
  std::function<void (std::stringstream &)> const &emplace_err
) {
  if (!rule.is_object()) {
    err << "invalid `rules` -> not an array of objects";
    emplace_err(err);
    return false;
  }

  if (rule.count("shade") == 0) {
    err << "invalid `rules` -> [" << index << "].shade not defined";
    emplace_err(err);
    return false;
  }

  if (rule.count("replacement") == 0) {
    err << "invalid `rules` -> [" << index << "].replacement not defined";
    emplace_err(err);
    return false;
  }

  if (rule.count("turn") == 0) {
    err << "invalid `rules` -> [" << index << "].turn not defined";
    emplace_err(err);
    return false;
  }

  uint8_t shade, replacement;
  ant::turn_direction::type turn_dir;

  try {
    if (!rule["shade"].is_number_unsigned()) {
      err << "invalid `rules` -> [" << index << "].shade is not an unsigned integer";
      emplace_err(err);
      return false;
    }

    uintmax_t const temp = rule["shade"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      err << "invalid `rules` -> [" << index << "].shade is > "
        // need to cast from uint8_t because operator<< overload doesn't treat it like a number
        << static_cast<int>(UINT8_MAX);
      emplace_err(err);
      return false;
    }

    shade = static_cast<uint8_t>(temp);

    if (out[shade].turn_dir != ant::turn_direction::NIL) {
      // rule already set
      err << "invalid `rules` -> more than one rule for shade " << shade;
      emplace_err(err);
      return false;
    }

  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `rules` -> [" << index << "].shade "
      << nlohmann_json_extract_sentence(except);
    emplace_err(err);
    return false;
  }

  try {
    if (!rule["replacement"].is_number_unsigned()) {
      err << "invalid `rules` -> [" << index << "].replacement is not an unsigned integer";
      emplace_err(err);
      return false;
    }

    uintmax_t const temp = rule["replacement"].get<uintmax_t>();

    if (temp > UINT8_MAX) {
      err << "invalid `rules` -> [" << index << "].replacement is > "
        // need to cast from uint8_t because operator<< overload doesn't treat it like a number
        << static_cast<int>(UINT8_MAX);
      emplace_err(err);
      return false;
    }

    replacement = rule["replacement"].get<uint8_t>();

  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `rules` -> [" << index << "].replacement "
      << nlohmann_json_extract_sentence(except);
    emplace_err(err);
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
      err << "invalid `rules` -> [" << index << "].turn is not one of L|N|R";
      emplace_err(err);
      return false;
    }

  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `rules` -> [" << index << "].turn "
      << nlohmann_json_extract_sentence(except);
    emplace_err(err);
    return false;
  }

  out[shade] = { replacement, turn_dir };
  return true;
}

static
bool try_to_parse_and_set_save_point(
  std::array<uint_fast64_t, 7> &out,
  json_t const &save_point,
  size_t const index,
  std::stringstream &err,
  std::function<void (std::stringstream &)> const &emplace_err
) {
  if (!save_point.is_number_unsigned()) {
    err << "invalid `save_points` -> [" << index << "] is not an unsigned integer";
    emplace_err(err);
    return false;
  }

  uint_fast64_t val;

  try {
    val = save_point.get<uint_fast64_t>();
  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `save_points` -> [" << index << "] "
      << nlohmann_json_extract_sentence(except);
    emplace_err(err);
    return false;
  }

  out[index] = val;
  return true;
}

static
bool try_to_parse_and_set_rules(
  json_t const &json,
  ant::simulation &sim,
  std::stringstream &err,
  std::function<void (std::stringstream &)> const &emplace_err
) {
  json_t::array_t rules;
  try {
    rules = json["rules"].get<json_t::array_t>();
  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `rules` -> " << nlohmann_json_extract_sentence(except);
    emplace_err(err);
    return false;
  }

  if (rules.size() > sim.rules.size()) {
    err << "invalid `rules` -> max 256 allowed, but got " << rules.size();
    emplace_err(err);
    return false;
  }

  std::array<ant::rule, 256> parsed_rules{};

  for (size_t i = 0; i < rules.size(); ++i) {
    if (!try_to_parse_and_set_rule(parsed_rules, rules[i], i, err, emplace_err)) {
      return false;
    }
  }

  // validation
  {
    std::array<uint16_t, 256> shade_occurences{};
    size_t num_defined_rules = 0;

    for (size_t i = 0; i < sim.rules.size(); ++i) {
      auto const &r = sim.rules[i];
      bool const is_rule_used = r.turn_dir != ant::turn_direction::NIL;
      if (is_rule_used) {
        ++num_defined_rules;
        ++shade_occurences[i];
        ++shade_occurences[r.replacement_shade];
      }
    }

    if (num_defined_rules < 2) {
      err << "invalid `rules` -> fewer than 2 defined";
      emplace_err(err);
      return false;
    }

    size_t num_non_zero_occurences = 0, num_non_two_occurences = 0;
    for (auto const occurencesOfShade : shade_occurences) {
      if (occurencesOfShade != 0) {
        ++num_non_zero_occurences;
        if (occurencesOfShade != 2)
          ++num_non_two_occurences;
      }
    }

    if (num_non_zero_occurences < 2 || num_non_two_occurences > 0) {
      err << "invalid `rules` -> don't form a closed chain";
      emplace_err(err);
      return false;
    }
  }

  sim.rules = parsed_rules;
  return true;
}

ant::simulation_parse_result_t ant::simulation_parse(std::string const &str) {
  ant::simulation sim{};
  std::vector<std::string> errors{};
  std::stringstream err{};

  auto const emplace_err = [&errors](std::stringstream &err) {
    errors.emplace_back(err.str());
    err.str(""); // clear buffer
    err.clear(); // clear stream state
  };

  json_t json;
  try {
    json = json_t::parse(str);
  } catch (json_t::basic_json::parse_error const &except) {
    err << except.what();
    emplace_err(err);
    return { sim, errors };
  } catch (...) {
    err << "unexpected error whilst trying to parse simulation";
    emplace_err(err);
    return { sim, errors };
  }

  if (!json.is_object()) {
    err << "parsed simulation is not a JSON object";
    emplace_err(err);
    return { sim, errors };
  }

  auto const validate_property_set = [&json, &emplace_err, &err](
    char const *const key
  ) {
    size_t const occurences = json.count(key);
    if (occurences == 0) {
      err << "`" << key << "` not set";
      emplace_err(err);
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
  bool const generations_parse_success = try_to_parse_and_set_uint<
    uint_fast64_t
  >(
    json, "generations", sim.generations,
    0, UINT_FAST64_MAX,
    err, emplace_err
  );

  bool const grid_width_parse_success = try_to_parse_and_set_uint<int>(
    json, "grid_width", sim.grid_width,
    0, UINT16_MAX,
    err, emplace_err
  );

  bool const grid_height_parse_success = try_to_parse_and_set_uint<int>(
    json, "grid_height", sim.grid_height,
    0, UINT16_MAX,
    err, emplace_err
  );

  bool const ant_col_parse_success = try_to_parse_and_set_uint<int>(
    json, "ant_col", sim.ant_col,
    0, UINT16_MAX,
    err, emplace_err
  );

  bool const ant_row_parse_success = try_to_parse_and_set_uint<int>(
    json, "ant_row", sim.ant_row,
    0, UINT16_MAX,
    err, emplace_err
  );

  if (
    grid_width_parse_success &&
    (sim.grid_width < 1 || sim.grid_width > UINT16_MAX)
  ) {
    err << "invalid `grid_width` -> not in range [1, " << UINT16_MAX << ']';
    emplace_err(err);
  } else if (
    ant_col_parse_success &&
    !util::in_range_incl_excl(sim.ant_col, 0, sim.grid_width)
  ) {
    err << "invalid `ant_col` -> not in grid x-axis [0, " << sim.grid_width << ')';
    emplace_err(err);
  }

  if (
    grid_height_parse_success &&
    (sim.grid_height < 1 || sim.grid_height > UINT16_MAX)
  ) {
    err << "invalid `grid_height` -> not in range [1, " << UINT16_MAX << ']';
    emplace_err(err);
  } else if (
    ant_row_parse_success &&
    !util::in_range_incl_excl(sim.ant_row, 0, sim.grid_height)
  ) {
    err << "invalid `ant_row` -> not in grid y-axis [0, " << sim.grid_height << ')';
    emplace_err(err);
  }

  [[maybe_unused]]
  bool const save_interval_parse_success = try_to_parse_and_set_uint<
    uint_fast64_t
  >(
    json, "save_interval", sim.save_interval,
    0, UINT_FAST64_MAX,
    err, emplace_err
  );

  [[maybe_unused]]
  bool const last_step_res_parse_success = try_to_parse_and_set_enum<
    ant::step_result::type
  >(json, "last_step_result", {
    { "nil", ant::step_result::NIL },
    { "success", ant::step_result::SUCCESS },
    { "failed_at_boundary", ant::step_result::FAILED_AT_BOUNDARY },
  }, sim.last_step_res, err, emplace_err);

  [[maybe_unused]]
  bool const ant_orientation_parse_success = try_to_parse_and_set_enum<
    ant::orientation::type
  >(json, "ant_orientation", {
    { "north", ant::orientation::NORTH },
    { "east", ant::orientation::EAST },
    { "south", ant::orientation::SOUTH },
    { "west", ant::orientation::WEST },
  }, sim.ant_orientation, err, emplace_err);

  [[maybe_unused]]
  bool const rules_parse_success = try_to_parse_and_set_rules(
    json, sim, err, emplace_err
  );

  {
    json_t::array_t save_points;

    try {
      save_points = json["save_points"].get<json_t::array_t>();

      size_t const num_save_points_specified = save_points.size();
      size_t const max_save_points_allowed = sim.save_points.size();

      if (num_save_points_specified > max_save_points_allowed) {
        err << "invalid `save_points` -> max " << max_save_points_allowed
          << " allowed, but got " << num_save_points_specified;
        emplace_err(err);
      } else {
        std::array<uint_fast64_t, 7> parsed_save_points{};

        bool success = true;
        for (size_t i = 0; i < num_save_points_specified; ++i) {
          if (!try_to_parse_and_set_save_point(parsed_save_points, save_points[i], i, err, emplace_err)) {
            success = false;
            break;
          }
        }

        if (success) {
          sim.save_points = parsed_save_points;
          sim.num_save_points = num_save_points_specified;
        }
      }
    } catch (json_t::basic_json::type_error const &except) {
      err << "invalid `save_points` -> " << nlohmann_json_extract_sentence(except);
      emplace_err(err);
    }
  }

  // save_points validation
  {
    std::unordered_map<
      uint_fast64_t, // point
      size_t // num of occurences
    > save_point_occurences{};

    // count how many times each save_point occurs
    for (size_t i = 0; i < sim.num_save_points; ++i)
      ++save_point_occurences[sim.save_points[i]];

    // check for repeats
    for (auto const [save_point, num_occurences] : save_point_occurences) {
      if (num_occurences > 1) {
        err << "save_point `" << save_point
          << "` repeated " << num_occurences << " times";
        emplace_err(err);
      }
    }

    // check for zeroes
    for (size_t i = 0; i < sim.num_save_points; ++i) {
      if (sim.save_points[i] == 0) {
        err << "save_point cannot be `0`";
        emplace_err(err);
        break;
      }
    }
  }

  try {
    std::string const val = json["grid_state"].get<std::string>();

    if (val == "") {
      err << "invalid `grid_state` -> cannot be blank";
      emplace_err(err);

    } else if (std::regex_match(val, std::regex("^fill -{1,}[0-9]{1,}$", std::regex_constants::icase))) {
      err << "invalid `grid_state` -> fill value cannot be negative";
      emplace_err(err);

    } else if (std::regex_match(val, std::regex("^fill [0-9]{1,}$", std::regex_constants::icase))) {
      char const *const num_str = val.c_str() + 4;
      uintmax_t const fill_val = strtoumax(num_str, nullptr, 10);
      if (fill_val > UINT8_MAX) {
        err << "invalid `grid_state` -> fill value must be <= "
          // need to cast from uint8_t because operator<< overload doesn't treat it like a number
          << static_cast<int>(UINT8_MAX);
        emplace_err(err);
      } else if (errors.empty()) {
        // only try to allocate and setup grid if there are no errors

        size_t const num_pixels = static_cast<size_t>(sim.grid_width) * sim.grid_height;
        try {
          sim.grid = new uint8_t[num_pixels];
          std::fill_n(sim.grid, num_pixels, static_cast<uint8_t>(fill_val));
        } catch (std::bad_alloc const &) {
          err << "failed to allocate " << num_pixels << " bytes for grid";
          emplace_err(err);
        }
      }
    } else {
      if (!std::filesystem::exists(val)) {
        err << "invalid `grid_state` -> file \"" << val << "\" does not exist";
        emplace_err(err);
      } else {
        std::ifstream file(val);
        if (!file.is_open()) {
          err << "unable to open file \"" << val << "\"";
          emplace_err(err);
        } else {
          size_t num_pixels = 0;
          try {
            auto const img_props = pgm8::read_properties(file);
            num_pixels = img_props.num_pixels();
            sim.grid = new uint8_t[num_pixels];
            pgm8::read_pixels(file, img_props, sim.grid);
          } catch (std::bad_alloc const &) {
            err << "failed to allocate " << num_pixels << " bytes for grid";
            emplace_err(err);
          } catch (std::runtime_error const &except) {
            err << "failed to read file \"" << val << "\" - " << except.what();
            emplace_err(err);
          }
        }
      }
    }
  } catch (json_t::basic_json::type_error const &except) {
    err << "invalid `grid_state` -> " << nlohmann_json_extract_sentence(except);
    emplace_err(err);
  }

  return { sim, errors };
}
