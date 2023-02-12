#include "lib/json.hpp"

#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::make_str;
using util::strvec_t;

static
u8 deduce_maxval(std::array<simulation::rule, 256> rules)
{
  u8 maxval = 0;

  for (usize i = 1; i < rules.size(); ++i) {
    if (rules[i].turn_dir != simulation::turn_direction::NIL)
      maxval = static_cast<u8>(i);
  }

  return maxval;
}

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

  {
    std::ofstream cfg_file(p);
    if (!cfg_file.is_open())
      throw std::runtime_error(make_str("failed to open file \"%s\"", p.string().c_str()));

    json_t json, temp;
    json_t::array_t rules_json, save_points_json;

    json["generation"] = state.generation;
    json["last_step_result"] = simulation::step_result::to_string(state.last_step_res);
    json["grid_width"] = state.grid_width;
    json["grid_height"] = state.grid_height;
    json["grid_state"] = (name_with_gen + ".pgm");
    json["ant_col"] = state.ant_col;
    json["ant_row"] = state.ant_row;
    json["ant_orientation"] = simulation::orientation::to_string(state.ant_orientation);

    for (usize shade = 0; shade < state.rules.size(); ++shade) {
      auto const &rule = state.rules[shade];
      if (rule.turn_dir != simulation::turn_direction::NIL) {
        temp["on"] = shade;
        temp["replace_with"] = rule.replacement_shade;
        temp["turn"] = turn_direction::to_string(rule.turn_dir);
        rules_json.push_back(temp);
        // temp.clear();
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
    img_props.set_width(static_cast<u16>(state.grid_width));
    img_props.set_height(static_cast<u16>(state.grid_height));
    img_props.set_maxval(deduce_maxval(state.rules));

    pgm8::write(img_file, img_props, state.grid);
  }
}
