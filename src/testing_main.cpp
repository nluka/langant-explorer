#include "lib/json.hpp"
#include "lib/ntest.hpp"

#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::strvec_t;

simulation::rules_t generate_rules(
  std::vector<std::pair<
    usize, // shade
    simulation::rule
  >> const &scheme)
{
  simulation::rules_t rules{};
  for (auto const [shade, rule] : scheme)
    rules[shade] = rule;
  return rules;
}

int main()
{
  ntest::init();

  {
    auto const assert_parse = [](
      simulation::state const &exp_state,
      strvec_t const &exp_errors,
      char const *const actual_pathname,
      std::source_location const loc = std::source_location::current())
    {
      std::string const json_str = util::extract_txt_file_contents(
        (std::string("parse/") + actual_pathname).c_str(),
        true
      );

      strvec_t act_errors{};

      auto const act_state = simulation::parse_state(
        json_str, (std::filesystem::current_path() / "parse"), act_errors
      );

      ntest::assert_uint64(exp_state.generation, act_state.generation, loc);
      ntest::assert_int32(exp_state.grid_width, act_state.grid_width, loc);
      ntest::assert_int32(exp_state.grid_height, act_state.grid_height, loc);
      ntest::assert_int32(exp_state.ant_col, act_state.ant_col, loc);
      ntest::assert_int32(exp_state.ant_row, act_state.ant_row, loc);
      ntest::assert_int8(exp_state.ant_orientation, act_state.ant_orientation, loc);
      ntest::assert_stdarr(exp_state.rules, act_state.rules, loc);
      ntest::assert_arr(
        exp_state.grid,
        exp_state.grid == nullptr ? 0 : static_cast<usize>(exp_state.grid_width) * exp_state.grid_height,
        act_state.grid,
        act_state.grid == nullptr ? 0 : static_cast<usize>(act_state.grid_width) * act_state.grid_height,
        loc
      );
      ntest::assert_stdvec(exp_errors, act_errors, loc);
    };

    assert_parse(
      {}, // state
      { // errors
        "invalid `generation` -> type must be number, but is string",
        "invalid `grid_width` -> type must be number, but is object",
        "invalid `grid_height` -> type must be number, but is array",
        "invalid `ant_col` -> type must be number, but is null",
        "invalid `ant_row` -> type must be number, but is string",
        "invalid `last_step_result` -> type must be string, but is number",
        "invalid `ant_orientation` -> type must be string, but is number",
        "invalid `rules` -> type must be array, but is object",
        "invalid `grid_state` -> type must be string, but is boolean",
      },
      "type_errors_0.json"
    );

    assert_parse(
      {}, // state
      { // errors
        "invalid `generation` -> type must be number, but is null",
        "invalid `grid_width` -> type must be number, but is array",
        "invalid `grid_height` -> type must be number, but is object",
        "invalid `ant_col` -> type must be number, but is string",
        "invalid `ant_row` -> type must be number, but is null",
        "invalid `last_step_result` -> type must be string, but is boolean",
        "invalid `ant_orientation` -> type must be string, but is boolean",
        "invalid `rules` -> not an array of objects",
        "invalid `grid_state` -> type must be string, but is number",
      },
      "type_errors_1.json"
    );

    assert_parse(
      {}, // state
      { // errors
        "`generation` not set",
        "`last_step_result` not set",
        "`grid_width` not set",
        "`grid_height` not set",
        "`grid_state` not set",
        "`ant_col` not set",
        "`ant_row` not set",
        "`ant_orientation` not set",
        "`rules` not set",
      },
      "occurence_errors.json"
    );

    assert_parse(
      {}, // state
      { // errors
        "invalid `generation` -> not an unsigned integer",
        "invalid `grid_width` -> not an unsigned integer",
        "invalid `grid_height` -> not an unsigned integer",
        "invalid `ant_col` -> not an unsigned integer",
        "invalid `ant_row` -> not an unsigned integer",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].on is not an unsigned integer",
        "invalid `grid_state` -> cannot be blank",
      },
      "value_errors_0.json"
    );

    assert_parse(
      {}, // state
      { // errors
        "invalid `grid_width` -> cannot be > 65535",
        "invalid `grid_height` -> cannot be > 65535",
        "invalid `ant_col` -> cannot be > 65535",
        "invalid `ant_row` -> cannot be > 65535",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].on is > 255",
        "invalid `grid_state` -> fill shade cannot be negative",
      },
      "value_errors_1.json"
    );

    assert_parse(
      { // state
        18446744073709551615, // generation
        65535, // grid_width
        65535, // grid_height
        65535, // ant_col
        65535, // ant_row
        nullptr, // grid
        simulation::orientation::NORTH,
        simulation::step_result::NIL,
        {}, // rules
      },
      { // errors
        "invalid `ant_col` -> not in grid x-axis [0, 65535)",
        "invalid `ant_row` -> not in grid y-axis [0, 65535)",
        "invalid `rules` -> [0].replace_with is not an unsigned integer",
        "invalid `grid_state` -> fill shade must be <= 255",
      },
      "value_errors_2.json"
    );

    assert_parse(
      { // state
        10, // generation
        0, // grid_width
        0, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::EAST,
        simulation::step_result::SUCCESS,
        {}, // rules
      },
      { // errors
        "invalid `grid_width` -> not in range [1, 65535]",
        "invalid `grid_height` -> not in range [1, 65535]",
        "invalid `rules` -> [0].replace_with is > 255",
        "invalid `grid_state` -> file \"non_existent_file\" does not exist",
      },
      "value_errors_3.json"
    );

    assert_parse(
      { // state
        20, // generation
        1, // grid_width
        1, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::SOUTH,
        simulation::step_result::HIT_EDGE,
        {}, // rules
      },
      { // errors
        "invalid `rules` -> [0].turn is not one of L|N|R",
        "invalid `grid_state` -> file \"fake_file\" does not exist",
      },
      "value_errors_4.json"
    );

    assert_parse(
      { // state
        20, // generation
        300, // grid_width
        200, // grid_height
        3, // ant_col
        6, // ant_row
        nullptr, // grid
        simulation::orientation::WEST,
        simulation::step_result::HIT_EDGE,
        {}, // rules
      },
      { // errors
        "invalid `rules` -> [0].turn is not one of L|N|R"
      },
      "value_errors_5.json"
    );

    assert_parse(
      { // state
        0, // generation
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::EAST,
        simulation::step_result::NIL,
        {}, // rules
      },
      { // errors
        "invalid `rules` -> fewer than 2 defined"
      },
      "value_errors_6.json"
    );

    assert_parse(
      { // state
        0, // generation
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::EAST,
        simulation::step_result::NIL,
        {}, // rules
      },
      { // errors
        "invalid `rules` -> don't form a closed chain",
      },
      "value_errors_7.json"
    );

    assert_parse(
      { // state
        0, // generation
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::EAST,
        simulation::step_result::NIL,
        generate_rules({
          // 0->1->2
          { 0, { 1, simulation::turn_direction::LEFT } },
          { 1, { 0, simulation::turn_direction::LEFT } },
        }),
      },
      { // errors
        "invalid `grid_state` -> fill shade has no governing rule",
      },
      "value_errors_8.json"
    );

    assert_parse(
      { // state
        0, // generation
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::EAST,
        simulation::step_result::NIL,
        {}, // rules
      },
      { // errors
        "invalid `rules` -> don't form a closed chain",
      },
      "value_errors_9.json"
    );

    {
      u8 grid[5 * 6];
      std::fill_n(grid, 5 * 6, 0ui8);

      assert_parse(
        { // state
          0, // generation
          5, // grid_width
          6, // grid_height
          2, // ant_col
          3, // ant_row
          grid,
          simulation::orientation::WEST,
          simulation::step_result::NIL,
          generate_rules({
            // 0->1->2
            { 0, { 1, simulation::turn_direction::LEFT } },
            { 1, { 0, simulation::turn_direction::RIGHT } },
          }),
        },
        {}, // errors
        "good_fill.json"
      );
    }

    {
      u8 grid[5 * 5]{
        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
      };

      assert_parse(
        { // state
          0, // generation
          5, // grid_width
          5, // grid_height
          2, // ant_col
          2, // ant_row
          grid,
          simulation::orientation::WEST,
          simulation::step_result::NIL,
          generate_rules({
            // 0->1->2
            { 0, { 1, simulation::turn_direction::LEFT } },
            { 1, { 2, simulation::turn_direction::NO_CHANGE } },
            { 2, { 0, simulation::turn_direction::RIGHT } },
          }),
        },
        {}, // errors
        "good_img.json"
      );
    }
  }

  {
    auto const assert_save_point = [](
      char const *const exp_json_name_cstr,
      char const *const exp_img_name_cstr,
      char const *const act_json_name_cstr,
      std::source_location const loc = std::source_location::current())
    {
      [[maybe_unused]] auto cwd = fs::current_path();

      std::string const base = "run/";

      std::string const
        exp_json_pathname = base + exp_json_name_cstr,
        act_json_pathname = base + act_json_name_cstr;

      std::string const
        exp_json_str = util::extract_txt_file_contents(exp_json_pathname.c_str(), false),
        act_json_str = util::extract_txt_file_contents(act_json_pathname.c_str(), false);

      json_t const
        exp_json = json_t::parse(exp_json_str),
        act_json = json_t::parse(act_json_str);

      b8 match =
        exp_json["generation"] == act_json["generation"] &&
        exp_json["last_step_result"] == act_json["last_step_result"] &&
        exp_json["grid_width"] == act_json["grid_width"] &&
        exp_json["grid_height"] == act_json["grid_height"] &&
        exp_json["ant_col"] == act_json["ant_col"] && exp_json["ant_row"] == act_json["ant_row"] &&
        exp_json["ant_orientation"] == act_json["ant_orientation"] &&
        exp_json["rules"].get<json_t::array_t>() == act_json["rules"].get<json_t::array_t>();

      std::string const act_img_pathname = base + act_json["grid_state"].get<std::string>();

      std::string const
        expected_img = util::extract_txt_file_contents((base + exp_img_name_cstr).c_str(), true),
        actual_img = util::extract_txt_file_contents(act_img_pathname.c_str(), true);

      match = match && (expected_img == actual_img);

      ntest::assert_bool(true, match, loc);
    };

    fs::path const save_dir = fs::current_path() / "run";
    std::string const json_str = util::extract_txt_file_contents("run/RL-init.json", false);
    strvec_t errors{};
    auto state = simulation::parse_state(json_str, save_dir, errors);

    ntest::assert_bool(true, errors.empty());

    if (errors.empty()) {
      simulation::run(
        state,
        "RL.actual",
        50, // generation_target
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::PLAIN,
        save_dir,
        true // save_final_state
      );

      assert_save_point("RL.expect(3).json", "RL.expect(3).pgm", "RL.actual(3).json");
      assert_save_point("RL.expect(16).json", "RL.expect(16).pgm", "RL.actual(16).json");
      assert_save_point("RL.expect(32).json", "RL.expect(32).pgm", "RL.actual(32).json");
      assert_save_point("RL.expect(48).json", "RL.expect(48).pgm", "RL.actual(48).json");
      assert_save_point("RL.expect(50).json", "RL.expect(50).pgm", "RL.actual(50).json");
    }
  }

  {
    std::string what_str;

    ntest::assert_throws<json_t::parse_error>(
      []() { [[maybe_unused]] auto const a = util::parse_json_array_u64("..."); });

    what_str = ntest::assert_throws<std::runtime_error>(
      []() { auto const a = util::parse_json_array_u64("{}"); });
    ntest::assert_cstr("not a JSON array", what_str.c_str());

    what_str = ntest::assert_throws<std::runtime_error>(
      []() { auto const a = util::parse_json_array_u64("[10, -100]"); });
    ntest::assert_cstr("element at idx 1 is not unsigned number", what_str.c_str());

    what_str = ntest::assert_throws<std::runtime_error>(
      []() { auto const a = util::parse_json_array_u64("[10, 20, {}]"); });
    ntest::assert_cstr("element at idx 2 is not unsigned number", what_str.c_str());

    ntest::assert_stdvec({}, util::parse_json_array_u64("[]"));

    ntest::assert_stdvec({ 10, 20, 30 }, util::parse_json_array_u64("[10, 20, 30]"));
  }

  ntest::generate_report("report");

  return 0;
}
