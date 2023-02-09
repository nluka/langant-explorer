#include "exit.hpp"
#include "json.hpp"
#include "ntest.hpp"
#include "simulation.hpp"
#include "util.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::strvec_t;

typedef std::array<simulation::rule, 256> rules_t;

rules_t
generate_rules(std::vector<std::pair<
                 usize, // shade
                 simulation::rule>> const &scheme)
{
  rules_t rules{};
  for (auto const [shade, rule] : scheme)
    rules[shade] = rule;
  return rules;
}

int
main()
{
  ntest::init();

  {
    auto const assert_parse = [](
                                simulation::configuration const &exp_cfg,
                                strvec_t const &exp_errors,
                                char const *const actual_pathname,
                                std::source_location const loc = std::source_location::current()) {
      std::string const json_str =
        util::extract_txt_file_contents((std::string("parse/") + actual_pathname).c_str());

      strvec_t act_errors{};

      auto const act_cfg =
        simulation::parse_config(json_str, (std::filesystem::current_path() / "parse"), act_errors);

      ntest::assert_uint64(exp_cfg.generation, act_cfg.generation, loc);
      ntest::assert_int32(exp_cfg.grid_width, act_cfg.grid_width, loc);
      ntest::assert_int32(exp_cfg.grid_height, act_cfg.grid_height, loc);
      ntest::assert_int32(exp_cfg.ant_col, act_cfg.ant_col, loc);
      ntest::assert_int32(exp_cfg.ant_row, act_cfg.ant_row, loc);
      ntest::assert_int8(exp_cfg.ant_orientation, act_cfg.ant_orientation, loc);
      ntest::assert_stdarr(exp_cfg.rules, act_cfg.rules, loc);
      ntest::assert_arr(
        exp_cfg.grid,
        exp_cfg.grid == nullptr ? 0 : static_cast<usize>(exp_cfg.grid_width) * exp_cfg.grid_height,
        act_cfg.grid,
        act_cfg.grid == nullptr ? 0 : static_cast<usize>(act_cfg.grid_width) * act_cfg.grid_height,
        loc);
      ntest::assert_stdvec(exp_errors, act_errors, loc);
    };

    assert_parse(
      {},
      {
        "invalid `generations` -> type must be number, but is string",
        "invalid `grid_width` -> type must be number, but is object",
        "invalid `grid_height` -> type must be number, but is array",
        "invalid `ant_col` -> type must be number, but is null",
        "invalid `ant_row` -> type must be number, but is string",
        "invalid `last_step_result` -> type must be string, but is number",
        "invalid `ant_orientation` -> type must be string, but is number",
        "invalid `rules` -> type must be array, but is object",
        "invalid `grid_state` -> type must be string, but is boolean",
      },
      "type_errors_0.json");

    assert_parse(
      {},
      {
        "invalid `generations` -> type must be number, but is null",
        "invalid `grid_width` -> type must be number, but is array",
        "invalid `grid_height` -> type must be number, but is object",
        "invalid `ant_col` -> type must be number, but is string",
        "invalid `ant_row` -> type must be number, but is null",
        "invalid `last_step_result` -> type must be string, but is boolean",
        "invalid `ant_orientation` -> type must be string, but is boolean",
        "invalid `rules` -> not an array of objects",
        "invalid `grid_state` -> type must be string, but is number",
      },
      "type_errors_1.json");

    assert_parse(
      {},
      {
        "`generations` not set",
        "`last_step_result` not set",
        "`grid_width` not set",
        "`grid_height` not set",
        "`grid_state` not set",
        "`ant_col` not set",
        "`ant_row` not set",
        "`ant_orientation` not set",
        "`rules` not set",
      },
      "occurence_errors.json");

    assert_parse(
      {},
      {
        "invalid `generations` -> not an unsigned integer",
        "invalid `grid_width` -> not an unsigned integer",
        "invalid `grid_height` -> not an unsigned integer",
        "invalid `ant_col` -> not an unsigned integer",
        "invalid `ant_row` -> not an unsigned integer",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].shade is not an unsigned integer",
        "invalid `grid_state` -> cannot be blank",
      },
      "value_errors_0.json");

    assert_parse(
      {},
      {
        "invalid `grid_width` -> cannot be > 65535",
        "invalid `grid_height` -> cannot be > 65535",
        "invalid `ant_col` -> cannot be > 65535",
        "invalid `ant_row` -> cannot be > 65535",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].shade is > 255",
        "invalid `grid_state` -> fill value cannot be negative",
      },
      "value_errors_1.json");

    assert_parse(
      {
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
      {
        "invalid `ant_col` -> not in grid x-axis [0, 65535)",
        "invalid `ant_row` -> not in grid y-axis [0, 65535)",
        "invalid `rules` -> [0].replacement is not an unsigned integer",
        "invalid `grid_state` -> fill value must be <= 255",
      },
      "value_errors_2.json");

    assert_parse(
      {
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
      {
        "invalid `grid_width` -> not in range [1, 65535]",
        "invalid `grid_height` -> not in range [1, 65535]",
        "invalid `rules` -> [0].replacement is > 255",
        "invalid `grid_state` -> file \"non_existent_file\" does not exist",
      },
      "value_errors_3.json");

    assert_parse(
      {
        20, // generation
        1, // grid_width
        1, // grid_height
        0, // ant_col
        0, // ant_row
        nullptr, // grid
        simulation::orientation::SOUTH,
        simulation::step_result::FAILED_AT_BOUNDARY,
        {}, // rules
      },
      {
        "invalid `rules` -> [0].turn is not one of L|N|R",
        "invalid `grid_state` -> file \"fake_file\" does not exist",
      },
      "value_errors_4.json");

    assert_parse(
      {
        20, // generation
        300, // grid_width
        200, // grid_height
        3, // ant_col
        6, // ant_row
        nullptr, // grid
        simulation::orientation::WEST,
        simulation::step_result::FAILED_AT_BOUNDARY,
        {}, // rules
      },
      { "invalid `rules` -> [0].turn is not one of L|N|R" },
      "value_errors_5.json");

    assert_parse(
      {
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
      { "invalid `rules` -> fewer than 2 defined" },
      "value_errors_6.json");

    assert_parse(
      {
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
      {
        "invalid `rules` -> don't form a closed chain",
      },
      "value_errors_7.json");

    assert_parse(
      {
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
      {
        "invalid `grid_state` -> fill value has no governing rule",
      },
      "value_errors_8.json");

    assert_parse(
      {
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
      {
        "invalid `rules` -> don't form a closed chain",
      },
      "value_errors_9.json");

    {
      u8 grid[5 * 6];
      std::fill_n(grid, 5 * 6, 0ui8);

      assert_parse(
        {
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
        {
          // no errors
        },
        "good_fill.json");
    }

    {
      u8 grid[5 * 5]{
        0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
      };

      assert_parse(
        {
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
        {
          // no errors
        },
        "good_img.json");
    }
  }

  {
    auto const assert_save_point = [](
                                     char const *const exp_json_name_cstr,
                                     char const *const act_json_name_cstr,
                                     std::source_location const loc =
                                       std::source_location::current()) {
      std::string const exp_json_pathname =
        std::string("run_with_save_points/") + exp_json_name_cstr;
      std::string const act_json_pathname =
        std::string("run_with_save_points/") + act_json_name_cstr;

      std::string const exp_json_str = util::extract_txt_file_contents(exp_json_pathname.c_str());
      std::string const act_json_str = util::extract_txt_file_contents(act_json_pathname.c_str());

      json_t const exp_json = json_t::parse(exp_json_str);
      json_t const act_json = json_t::parse(act_json_str);

      b8 match = exp_json["generations"] == act_json["generations"] &&
        exp_json["last_step_result"] == act_json["last_step_result"] &&
        exp_json["grid_width"] == act_json["grid_width"] &&
        exp_json["grid_height"] == act_json["grid_height"] &&
        exp_json["ant_col"] == act_json["ant_col"] && exp_json["ant_row"] == act_json["ant_row"] &&
        exp_json["ant_orientation"] == act_json["ant_orientation"] &&
        exp_json["rules"].get<json_t::array_t>() == act_json["rules"].get<json_t::array_t>();

      std::string const expected_img = util::extract_txt_file_contents(
        (std::string("run_with_save_points/") + exp_json["grid_state"].get<std::string>()).c_str());
      std::string const actual_img = util::extract_txt_file_contents(
        (std::string("run_with_save_points/") + act_json["grid_state"].get<std::string>()).c_str());

      match = match && (expected_img == actual_img);

      ntest::assert_bool(true, match, loc);
    };

    fs::path const save_dir = fs::current_path() / "run_with_save_points";

    std::string const json_str =
      util::extract_txt_file_contents("run_with_save_points/RL-init.json");

    strvec_t errors{};

    auto cfg = simulation::parse_config(json_str, save_dir, errors);

    ntest::assert_bool(true, errors.empty());

    if (!errors.empty()) {
      simulation::run(
        cfg,
        "RL.actual",
        50, // generation_target
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::PLAIN,
        save_dir,
        true // save_final_state
      );

      assert_save_point("RL.expected(3).json", "RL.actual(3).json");
      assert_save_point("RL.expected(16).json", "RL.actual(16).json");
      assert_save_point("RL.expected(32).json", "RL.actual(32).json");
      assert_save_point("RL.expected(48).json", "RL.actual(48).json");
      assert_save_point("RL.expected(50).json", "RL.actual(50).json");
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
