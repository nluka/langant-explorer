#include "lib/json.hpp"
#include "lib/ntest.hpp"
#include "lib/regexglob.hpp"

#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::errors_t;

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
      std::variant<simulation::state, errors_t> const &expected,
      char const *const state_path,
      std::source_location const loc = std::source_location::current())
    {
      std::string const json_str = util::extract_txt_file_contents(
        std::string("parse/") + state_path,
        true
      );

      auto const parse_res = simulation::parse_state(
        json_str, (std::filesystem::current_path() / "parse")
      );

      if (std::holds_alternative<simulation::state>(expected)) {
        // we expect the parse to have been successful...
        assert(std::holds_alternative<simulation::state>(parse_res));

        auto const &expected_state = std::get<simulation::state>(expected);
        auto const &actual_state = std::get<simulation::state>(parse_res);

        ntest::assert_uint64(expected_state.generation, actual_state.generation, loc);
        ntest::assert_int32(expected_state.grid_width, actual_state.grid_width, loc);
        ntest::assert_int32(expected_state.grid_height, actual_state.grid_height, loc);
        ntest::assert_int32(expected_state.ant_col, actual_state.ant_col, loc);
        ntest::assert_int32(expected_state.ant_row, actual_state.ant_row, loc);
        ntest::assert_int8(expected_state.ant_orientation, actual_state.ant_orientation, loc);
        ntest::assert_stdarr(expected_state.rules, actual_state.rules, loc);
        ntest::assert_arr(
          expected_state.grid,
          (
            expected_state.grid == nullptr ? 0
            : static_cast<usize>(expected_state.grid_width) * expected_state.grid_height
          ),
          actual_state.grid,
          (
            actual_state.grid == nullptr ? 0
            : static_cast<usize>(actual_state.grid_width) * actual_state.grid_height
          ),
          loc
        );

      } else {
        // we expect the parse to NOT have been successful...
        assert(std::holds_alternative<errors_t>(parse_res));

        errors_t const
          &expected_errors = std::get<errors_t>(expected),
          &actual_errors = std::get<errors_t>(parse_res);

        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      }
    };

    typedef std::variant<simulation::state, errors_t> state_or_errors_t;

    {
      errors_t expected_errors {
        "`generation` not set",
        "`last_step_result` not set",
        "`grid_width` not set",
        "`grid_height` not set",
        "`grid_state` not set",
        "`ant_col` not set",
        "`ant_row` not set",
        "`ant_orientation` not set",
        "`rules` not set",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "occurence_errors.json");
    }

    {
      errors_t expected_errors {
        "invalid `generation` -> type must be number, but is string",
        "invalid `grid_width` -> type must be number, but is object",
        "invalid `grid_height` -> type must be number, but is array",
        "invalid `ant_col` -> type must be number, but is null",
        "invalid `ant_row` -> type must be number, but is string",
        "invalid `last_step_result` -> type must be string, but is number",
        "invalid `ant_orientation` -> type must be string, but is number",
        "invalid `rules` -> type must be array, but is object",
        "invalid `grid_state` -> type must be string, but is boolean",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "type_errors_0.json");
    }

    {
      errors_t expected_errors {
        "invalid `generation` -> type must be number, but is null",
        "invalid `grid_width` -> type must be number, but is array",
        "invalid `grid_height` -> type must be number, but is object",
        "invalid `ant_col` -> type must be number, but is string",
        "invalid `ant_row` -> type must be number, but is null",
        "invalid `last_step_result` -> type must be string, but is boolean",
        "invalid `ant_orientation` -> type must be string, but is boolean",
        "invalid `rules` -> not an array of objects",
        "invalid `grid_state` -> type must be string, but is number",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "type_errors_1.json");
    }

    {
      errors_t expected_errors {
        "invalid `generation` -> not an unsigned integer",
        "invalid `grid_width` -> not an unsigned integer",
        "invalid `grid_height` -> not an unsigned integer",
        "invalid `ant_col` -> not an unsigned integer",
        "invalid `ant_row` -> not an unsigned integer",
        "invalid `last_step_result` -> not one of nil|success|hit_edge",
        "invalid `ant_orientation` -> not one of N|E|S|W",
        "invalid `rules` -> [0].on is not an unsigned integer",
        "invalid `grid_state` -> cannot be blank",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_0.json");
    }

    {
      errors_t expected_errors {
        "invalid `grid_width` -> cannot be > 65535",
        "invalid `grid_height` -> cannot be > 65535",
        "invalid `ant_col` -> cannot be > 65535",
        "invalid `ant_row` -> cannot be > 65535",
        "invalid `last_step_result` -> not one of nil|success|hit_edge",
        "invalid `ant_orientation` -> not one of N|E|S|W",
        "invalid `rules` -> [0].on is > 255",
        "invalid `grid_state` -> fill shade cannot be negative",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_1.json");
    }

    {
      errors_t expected_errors {
        "invalid `ant_col` -> not in grid x-axis [0, 65535)",
        "invalid `ant_row` -> not in grid y-axis [0, 65535)",
        "invalid `rules` -> [0].replace_with is not an unsigned integer",
        "invalid `grid_state` -> fill shade must be <= 255",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_2.json");
    }

    {
      errors_t expected_errors {
        "invalid `grid_width` -> not in range [1, 65535]",
        "invalid `grid_height` -> not in range [1, 65535]",
        "invalid `rules` -> [0].replace_with is > 255",
        "invalid `grid_state` -> file \"non_existent_file\" does not exist",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_3.json");
    }

    {
      errors_t expected_errors {
        "invalid `rules` -> [0].turn not recognized",
        "invalid `grid_state` -> file \"fake_file\" does not exist",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_4.json");
    }

    {
      errors_t expected_errors {
        "invalid `rules` -> [0].turn is empty"
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_5.json");
    }

    {
      errors_t expected_errors {
        "invalid `rules` -> fewer than 2 defined"
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_6.json");
    }

    {
      errors_t expected_errors {
        "invalid `rules` -> don't form a closed chain",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_7.json");
    }

    {
      errors_t expected_errors {
        "invalid `grid_state` -> fill shade has no governing rule",
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_8.json");
    }

    {
      errors_t expected_errors {
        "invalid `rules` -> don't form a closed chain"
      };
      assert_parse(state_or_errors_t(std::move(expected_errors)), "value_errors_9.json");
    }

    {
      u8 grid[5 * 6];
      std::fill_n(grid, 5 * 6, 0ui8);

      simulation::state expected_state {
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
      };

      assert_parse(state_or_errors_t(expected_state), "good_fill.json");
    }

    {
      u8 grid[5 * 5]{
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
      };

      simulation::state expected_state {
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
      };

      assert_parse(state_or_errors_t(expected_state), "good_img.json");
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
        exp_json_str = util::extract_txt_file_contents(exp_json_pathname, false),
        act_json_str = util::extract_txt_file_contents(act_json_pathname, false);

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
        expected_img = util::extract_txt_file_contents(base + exp_img_name_cstr, true),
        actual_img = util::extract_txt_file_contents(act_img_pathname, true);

      match = match && (expected_img == actual_img);

      ntest::assert_bool(true, match, loc);
    };

    fs::path const save_dir = fs::current_path() / "run";
    std::string const json_str = util::extract_txt_file_contents("run/RL-init.json", false);

    auto parse_res = simulation::parse_state(json_str, save_dir);
    assert(std::holds_alternative<simulation::state>(parse_res));

    simulation::state &state = std::get<simulation::state>(parse_res);

    auto const to_delete = regexglob::fmatch(save_dir.string().c_str(), "RL\\.actual.*");
    for (auto const &file : to_delete) {
      fs::remove_all(file);
    }

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

  ntest::generate_report("report");

  return 0;
}
