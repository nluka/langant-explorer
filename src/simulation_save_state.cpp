#include "lib/json.hpp"

#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::make_str;

void simulation::save_state(
  simulation::state const &state,
  const char *const name,
  fs::path const &dir,
  pgm8::format const fmt)
{
  if (!fs::is_directory(dir)) {
    throw std::runtime_error(make_str("\"%s\" is not a directory", dir.string().c_str()));
  }

  std::string const name_with_gen = make_str("%s(%zu)", name, state.generation);

  std::filesystem::path p(dir);
  p /= name_with_gen + ".json";

  // write state file
  {
    std::string const state_path_str = p.string();
    std::fstream state_file = util::open_file(state_path_str, std::ios::out);

    print_state_json(
      state_file,
      state.generation,
      state.last_step_res,
      state.grid_width,
      state.grid_height,
      name_with_gen + ".pgm",
      state.ant_col,
      state.ant_row,
      state.ant_orientation,
      state.rules);
  }

  {
    pgm8::image_properties img_props;
    img_props.set_format(fmt);
    img_props.set_width(static_cast<u16>(state.grid_width));
    img_props.set_height(static_cast<u16>(state.grid_height));
    img_props.set_maxval(state.maxval);

    p.replace_extension(".pgm");
    std::string const img_path_str = p.string();
    std::fstream img_file = util::open_file(img_path_str, std::ios::out);
    pgm8::write(img_file, img_props, state.grid);
  }
}

void simulation::print_state_json(
  std::ostream &os,
  u64 const generation,
  step_result::value_type const last_step_res,
  i32 const grid_width,
  i32 const grid_height,
  std::string const &grid_state,
  i32 const ant_col,
  i32 const ant_row,
  orientation::value_type const ant_orientation,
  rules_t const &rules)
{
  os
    << "{\n"
    << "  \"generation\": " << generation << ",\n"
    << "  \"last_step_result\": \"" << simulation::step_result::to_string(last_step_res) << "\",\n"
    << "\n"
    << "  \"grid_width\": " << grid_width << ",\n"
    << "  \"grid_height\": " << grid_height << ",\n"
    << "  \"grid_state\": \"" << grid_state << "\",\n"
    << "\n"
    << "  \"ant_col\": " << ant_col << ",\n"
    << "  \"ant_row\": " << ant_row << ",\n"
    << "  \"ant_orientation\": \"" << simulation::orientation::to_string(ant_orientation) << "\",\n"
    << "\n"
    << "  \"rules\": [\n"
  ;

  // rules elements
  {
    auto const print_rule = [&os](
      usize const shade,
      simulation::rule const &rule,
      bool const comma_at_end
    ) {
      os
        << "    { \"on\": " << shade
        << ", \"replace_with\": " << static_cast<u16>(rule.replacement_shade)
        << ", \"turn\": \"" << simulation::turn_direction::to_string(rule.turn_dir)
        << "\" }" << (comma_at_end ? "," : "") << '\n'
      ;
    };

    usize last_rule_idx = 0;
    for (usize i = 255; i > 0; --i) {
      if (rules[i].turn_dir != simulation::turn_direction::NIL) {
        last_rule_idx = i;
        break;
      }
    }

    for (usize shade = 0; shade < last_rule_idx; ++shade) {
      auto const &rule = rules[shade];
      bool const exists = rule.turn_dir != simulation::turn_direction::NIL;
      if (exists)
        print_rule(shade, rule, true);
    }
    print_rule(last_rule_idx, rules[last_rule_idx], false);
  }

  os
    << "  ]\n"
    << "}\n"
  ;
}
