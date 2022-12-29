#include "ant.hpp"
#include "arr2d.hpp"
#include "exit.hpp"
#include "ntest.hpp"
#include "seqgen.hpp"
#include "util.hpp"

typedef std::array<ant::rule, 256> rules_t;

rules_t generate_rules(std::vector<std::pair<
  size_t, // shade
  ant::rule
>> const &scheme) {
  rules_t rules{};
  for (auto const [shade, rule] : scheme)
    rules[shade] = rule;
  return rules;
}

int main() {
  ntest::init();

  {
    auto const assert_parse = [](
      ant::simulation_parse_result_t const &expected, char const *const actual_pathname,
      std::source_location const loc = std::source_location::current()
    ) {
      std::string content = util::extract_txt_file_contents(actual_pathname);
      auto const &[act_sim, act_errors] = ant::simulation_parse(content);
      auto const &[exp_sim, exp_errors] = expected;

      ntest::assert_uint64(exp_sim.generations, act_sim.generations, loc);
      ntest::assert_int32(exp_sim.grid_width, act_sim.grid_width, loc);
      ntest::assert_int32(exp_sim.grid_height, act_sim.grid_height, loc);
      ntest::assert_int32(exp_sim.ant_col, act_sim.ant_col, loc);
      ntest::assert_int32(exp_sim.ant_row, act_sim.ant_row, loc);
      ntest::assert_int8(exp_sim.ant_orientation, act_sim.ant_orientation, loc);
      ntest::assert_stdarr(exp_sim.rules, act_sim.rules, loc);
      ntest::assert_arr(
        exp_sim.grid,
        exp_sim.grid == nullptr ? 0
          : static_cast<size_t>(exp_sim.grid_width) * exp_sim.grid_height,
        act_sim.grid, act_sim.grid == nullptr ? 0
          : static_cast<size_t>(act_sim.grid_width) * act_sim.grid_height,
        loc
      );
      ntest::assert_stdvec(exp_errors, act_errors, loc);
    };

    assert_parse({
      {}, {
        "invalid `generations` -> type must be number, but is string",
        "invalid `grid_width` -> type must be number, but is object",
        "invalid `grid_height` -> type must be number, but is array",
        "invalid `ant_col` -> type must be number, but is null",
        "invalid `ant_row` -> type must be number, but is string",
        "invalid `save_interval` -> type must be number, but is array",
        "invalid `last_step_result` -> type must be string, but is number",
        "invalid `ant_orientation` -> type must be string, but is number",
        "invalid `rules` -> type must be array, but is object",
        "invalid `save_points` -> type must be array, but is boolean",
        "invalid `grid_state` -> type must be string, but is boolean",
      }
    }, "type_errors_0.json");

    assert_parse({
      {}, {
        "invalid `generations` -> type must be number, but is null",
        "invalid `grid_width` -> type must be number, but is array",
        "invalid `grid_height` -> type must be number, but is object",
        "invalid `ant_col` -> type must be number, but is string",
        "invalid `ant_row` -> type must be number, but is null",
        "invalid `save_interval` -> type must be number, but is string",
        "invalid `last_step_result` -> type must be string, but is boolean",
        "invalid `ant_orientation` -> type must be string, but is boolean",
        "invalid `rules` -> not an array of objects",
        "invalid `save_points` -> [0] is not an unsigned integer",
        "invalid `grid_state` -> type must be string, but is number",
      }
    }, "type_errors_1.json");

    assert_parse({
      {}, {
        "`generations` not set",
        "`last_step_result` not set",
        "`grid_width` not set",
        "`grid_height` not set",
        "`grid_state` not set",
        "`ant_col` not set",
        "`ant_row` not set",
        "`ant_orientation` not set",
        "`rules` not set",
        "`save_interval` not set",
        "`save_points` not set",
      }
    }, "occurence_errors.json");

    assert_parse({
      {}, {
        "invalid `generations` -> not an unsigned integer",
        "invalid `grid_width` -> not an unsigned integer",
        "invalid `grid_height` -> not an unsigned integer",
        "invalid `ant_col` -> not an unsigned integer",
        "invalid `ant_row` -> not an unsigned integer",
        "invalid `save_interval` -> not an unsigned integer",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].shade is not an unsigned integer",
        "invalid `save_points` -> [1] is not an unsigned integer",
        "invalid `grid_state` -> cannot be blank",
      }
    }, "value_errors_0.json");

    assert_parse({
      {}, {
        "invalid `grid_width` -> cannot be > 65535",
        "invalid `grid_height` -> cannot be > 65535",
        "invalid `ant_col` -> cannot be > 65535",
        "invalid `ant_row` -> cannot be > 65535",
        "invalid `last_step_result` -> not one of nil|success|failed_at_boundary",
        "invalid `ant_orientation` -> not one of north|east|south|west",
        "invalid `rules` -> [0].shade is > 255",
        "invalid `save_points` -> [0] is not an unsigned integer",
        "invalid `grid_state` -> fill value cannot be negative",
      }
    }, "value_errors_1.json");

    assert_parse({
      {
        18446744073709551615, // generations
        ant::step_result::NIL,
        65535, // grid_width
        65535, // grid_height
        65535, // ant_col
        65535, // ant_row
        ant::orientation::NORTH,
        nullptr, // grid
        {}, // rules
        0, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `ant_col` -> not in grid x-axis [0, 65535)",
        "invalid `ant_row` -> not in grid y-axis [0, 65535)",
        "invalid `rules` -> [0].replacement is not an unsigned integer",
        "invalid `save_points` -> [2] is not an unsigned integer",
        "invalid `grid_state` -> fill value must be <= 255",
      }
    }, "value_errors_2.json");

    assert_parse({
      {
        10, // generations
        ant::step_result::SUCCESS,
        0, // grid_width
        0, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::EAST,
        nullptr, // grid
        {}, // rules
        9223372036854775806, // save_interval
        3, // num_save_points
        {10, 20, 30}, // save_points
      }, {
        "invalid `grid_width` -> not in range [1, 65535]",
        "invalid `grid_height` -> not in range [1, 65535]",
        "invalid `rules` -> [0].replacement is > 255",
        "invalid `grid_state` -> file \"non_existent_file\" does not exist",
      }
    }, "value_errors_3.json");

    assert_parse({
      {
        20, // generations
        ant::step_result::FAILED_AT_BOUNDARY,
        1, // grid_width
        1, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::SOUTH,
        nullptr, // grid
        {}, // rules
        10, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `rules` -> [0].turn is not one of L|N|R",
        "invalid `grid_state` -> file \"fake_file\" does not exist",
      }
    }, "value_errors_4.json");

    assert_parse({
      {
        20, // generations
        ant::step_result::FAILED_AT_BOUNDARY,
        300, // grid_width
        200, // grid_height
        3, // ant_col
        6, // ant_row
        ant::orientation::WEST,
        nullptr, // grid
        {}, // rules
        10, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `rules` -> [0].turn is not one of L|N|R"
      }
    }, "value_errors_5.json");

    assert_parse({
      {
        0, // generations
        ant::step_result::NIL,
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::EAST,
        nullptr, // grid
        {}, // rules
        10, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `rules` -> fewer than 2 defined"
      }
    }, "value_errors_6.json");

    assert_parse({
      {
        0, // generations
        ant::step_result::NIL,
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::EAST,
        nullptr, // grid
        {}, // rules
        10, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `rules` -> don't form a closed chain",
      }
    }, "value_errors_7.json");

    assert_parse({
      {
        0, // generations
        ant::step_result::NIL,
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::EAST,
        nullptr, // grid
        generate_rules({
          // 0->1->2
          { 0, {1, ant::turn_direction::LEFT} },
          { 1, {0, ant::turn_direction::LEFT} },
        }),
        10, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `grid_state` -> fill value has no governing rule",
      }
    }, "value_errors_8.json");

    assert_parse({
      {
        0, // generations
        ant::step_result::NIL,
        5, // grid_width
        6, // grid_height
        0, // ant_col
        0, // ant_row
        ant::orientation::EAST,
        nullptr, // grid
        {}, // rules
        0, // save_interval
        0, // num_save_points
        {}, // save_points
      }, {
        "invalid `rules` -> don't form a closed chain",
      }
    }, "value_errors_9.json");

    {
      uint8_t grid[5 * 6];
      std::fill_n(grid, 5 * 6, 0ui8);

      assert_parse({
        {
          0, // generations
          ant::step_result::NIL,
          5, // grid_width
          6, // grid_height
          2, // ant_col
          3, // ant_row
          ant::orientation::WEST,
          grid,
          generate_rules({
            // 0->1->2
            { 0, {1, ant::turn_direction::LEFT} },
            { 1, {0, ant::turn_direction::RIGHT} },
          }),
          10, // save_interval
          1, // num_save_points
          {15}, // save_points
        }, {
          // no errors
        }
      }, "good_fill.json");
    }

    {
      uint8_t grid[5 * 5] {
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
      };

      assert_parse({
        {
          0, // generations
          ant::step_result::NIL,
          5, // grid_width
          5, // grid_height
          2, // ant_col
          2, // ant_row
          ant::orientation::WEST,
          &grid[0],
          generate_rules({
            // 0->1->2
            { 0, {1, ant::turn_direction::LEFT} },
            { 1, {2, ant::turn_direction::NONE} },
            { 2, {0, ant::turn_direction::RIGHT} },
          }),
          0, // save_interval
          0, // num_save_points
          {}, // save_points
        }, {
          // no errors
        }
      }, "good_img.json");
    }
  }

  ntest::generate_report("report");

  return 0;
}
