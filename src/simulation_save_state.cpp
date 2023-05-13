#include "core_source_libs.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::make_str;

simulation::save_state_result simulation::save_state(
  simulation::state const &state,
  const char *const name,
  fs::path const &save_dir,
  pgm8::format const fmt,
  b8 const image_only)
{
  if (!fs::is_directory(save_dir)) {
    throw std::runtime_error(make_str("save_dir '%s' is not a directory", save_dir.generic_string().c_str()));
  }

  save_state_result result{};

  std::string const name_with_gen = make_str("%s(%zu)", name, state.generation);

  std::filesystem::path file_path = save_dir / (name_with_gen + ".json");

  if (!image_only) {
    // write state file

    std::string const state_file_path_str = file_path.generic_string();
    std::fstream state_file = util::open_file(state_file_path_str, std::ios::out);

    b8 const success = print_state_json(
      state_file,
      state_file_path_str,
      name_with_gen + ".pgm",
      state.generation,
      state.grid_width,
      state.grid_height,
      state.ant_col,
      state.ant_row,
      state.last_step_res,
      state.ant_orientation,
      util::count_digits(state.maxval),
      state.rules);

    result.state_write_succes = success;
  }

  // write image file
  {
    pgm8::image_properties img_props;
    img_props.set_format(fmt);
    img_props.set_width(static_cast<u16>(state.grid_width));
    img_props.set_height(static_cast<u16>(state.grid_height));
    img_props.set_maxval(state.maxval);

    file_path.replace_extension(".pgm");
    std::string const img_path_str = file_path.string();
    std::fstream img_file = util::open_file(img_path_str, std::ios::out);
    b8 const success = pgm8::write(img_file, img_props, state.grid);

    result.image_write_success = success;
  }

  return result;
}

bool simulation::print_state_json(
  std::ostream &os,
  std::string const &file_path,
  std::string const &grid_state,
  u64 const generation,
  i32 const grid_width,
  i32 const grid_height,
  i32 const ant_col,
  i32 const ant_row,
  step_result::value_type const last_step_res,
  orientation::value_type const ant_orientation,
  u64 const maxval_digits,
  rules_t const &rules)
{
  using logger::log;
  using logger::event_type;

  os
    << "{\n"
    << "  \"generation\": " << generation << ",\n"
    << "  \"last_step_result\": \"" << simulation::step_result::to_cstr(last_step_res) << "\",\n"
    << "\n"
    << "  \"grid_width\": " << grid_width << ",\n"
    << "  \"grid_height\": " << grid_height << ",\n"
    << "  \"grid_state\": \"" << fs::path(grid_state).generic_string() << "\",\n"
    << "\n"
    << "  \"ant_col\": " << ant_col << ",\n"
    << "  \"ant_row\": " << ant_row << ",\n"
    << "  \"ant_orientation\": \"" << simulation::orientation::to_cstr(ant_orientation) << "\",\n"
    << "\n"
    << "  \"rules\": [\n"
  ;

  // rules elements
  {
    auto const print_rule = [&](
      u64 const shade,
      simulation::rule const &rule,
      b8 const comma_at_end)
    {
      os
        << "    " // indentation
        << "{ "
        << "\"on\": " << std::setw(maxval_digits) << std::setfill(' ') << shade << ", "
        << "\"replace_with\": " << std::setw(maxval_digits) << std::setfill(' ') << static_cast<u16>(rule.replacement_shade) << ", "
        << "\"turn\": \"" << simulation::turn_direction::to_cstr(rule.turn_dir) << "\""
        << " }" << (comma_at_end ? "," : "") << '\n'
      ;
    };

    u64 last_rule_idx = 0;
    for (u64 i = 255; i > 0; --i) {
      if (rules[i].turn_dir != simulation::turn_direction::NIL) {
        last_rule_idx = i;
        break;
      }
    }

    for (u64 shade = 0; shade < last_rule_idx; ++shade) {
      auto const &rule = rules[shade];
      b8 const exists = rule.turn_dir != simulation::turn_direction::NIL;
      if (exists)
        print_rule(shade, rule, true);
    }
    print_rule(last_rule_idx, rules[last_rule_idx], false);
  }

  os
    << "  ]\n"
    << "}\n"
  ;

  if (os.bad()) {
    log(event_type::ERR, "failed to write '%s', maybe not enough disk space?", file_path.c_str());
    return false;
  } else {
    return true;
  }
}
