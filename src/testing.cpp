#include "arr2d.hpp"
#include "exit.hpp"
#include "ntest.hpp"
#include "seqgen.hpp"
#include "simulation.hpp"
#include "util.hpp"

typedef std::array<rule, 256> Rules;

Rules generate_rules(std::vector<std::pair<
  size_t, // shade
  rule
>> const &scheme) {
  Rules rules{};
  for (auto const [shade, rule] : scheme)
    rules[shade] = rule;
  return rules;
}

int main() {
  ntest::assert_stdvec({
    // no errors
  }, validate_simulation({
    0, // generations
    step_result::NIL,
    5, // grid_width
    10, // grid_height
    3, // ant_col
    6, // ant_row
    orientation::NORTH,
    nullptr, // grid
    generate_rules({
      { 0, {1, turn_direction::RIGHT} },
      { 1, {0, turn_direction::LEFT} },
    }),
    0, // save_interval
    {}, // save_points
  }));

  ntest::assert_stdvec({
    "grid_width `-1` not in range [1, 65535]",
    "grid_height `70000` not in range [1, 65535]",
    "ant_orientation `5` not in range [1, 4]",
    "fewer than 2 rules defined",
  }, validate_simulation({
    0, // generations
    step_result::NIL,
    -1, // grid_width
    70'000, // grid_height
    10, // ant_col
    10, // ant_row
    orientation::OVERFLOW_CLOCKWISE,
    nullptr, // grid
    generate_rules({
      // 1->0
      // ^__|
      { 1, {0, turn_direction::LEFT} },
    }),
    0, // save_interval
    3, // num_save_points
    { 1, 2, 3, }, // save_points
  }));

  ntest::assert_stdvec({
    "ant_col `-1` not in grid x-axis [0, 5)",
    "ant_row `11` not in grid y-axis [0, 10)",
    "ant_orientation `0` not in range [1, 4]",
    "rules don't form a closed chain",
    "save_point `0` repeated 2 times",
    "save_point `1` repeated 3 times",
    "save_point `2` repeated 2 times",
    "save_point cannot be `0`",
  }, validate_simulation({
    0, // generations
    step_result::NIL,
    5, // grid_width
    10, // grid_height
    -1, // ant_col
    11, // ant_row
    orientation::OVERFLOW_COUNTER_CLOCKWISE,
    nullptr, // grid
    generate_rules({
      // 0->1->2
      { 0, {1, turn_direction::LEFT} },
      { 1, {2, turn_direction::LEFT} },
    }),
    0, // save_interval
    7, // num_save_points
    { // save_points
      0, 0,
      1, 1, 1,
      2, 2,
    },
  }));

  {
    auto const assert_parse = [](
      parse_result_t const &expected, char const *const actual_pathname,
      std::source_location const loc = std::source_location::current()
    ) {
      std::string content = util::extract_txt_file_contents(actual_pathname);
      auto const &[act_sim, act_errors] = parse_simulation(content);
      auto const &[exp_sim, exp_errors] = expected;

      ntest::assert_uint64(exp_sim.generations, act_sim.generations, loc);
      ntest::assert_int32(exp_sim.grid_width, act_sim.grid_width, loc);
      ntest::assert_int32(exp_sim.grid_height, act_sim.grid_height, loc);
      ntest::assert_int32(exp_sim.ant_col, act_sim.ant_col, loc);
      ntest::assert_int32(exp_sim.ant_row, act_sim.ant_row, loc);
      ntest::assert_int8(exp_sim.ant_orientation, act_sim.ant_orientation, loc);
      ntest::assert_stdarr(exp_sim.rules, act_sim.rules, loc);
      // ntest::assert_arr(
      //   exp_sim.grid, static_cast<size_t>(exp_sim.grid_width) * exp_sim.grid_height,
      //   act_sim.grid, static_cast<size_t>(act_sim.grid_width) * act_sim.grid_height,
      //   loc
      // );
      ntest::assert_stdvec(exp_errors, act_errors, loc);
    };

    // simulation_parse()

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
        "invalid `grid_state` -> type must be string, but is boolean",
        "invalid `rules` -> type must be array, but is object",
        "invalid `save_points` -> type must be array, but is boolean",
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
        "invalid `grid_state` -> type must be string, but is number",
        "invalid `rules` -> not an array of objects",
        "invalid `save_points` -> [0] is not an unsigned integer",
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
        "invalid `grid_state` -> cannot be blank",
        "invalid `rules` -> [0].shade is not an unsigned integer",
        "invalid `save_points` -> [1] is not an unsigned integer",
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
        "invalid `grid_state` -> fill value cannot be negative",
        "invalid `rules` -> [0].shade is > 255",
        "invalid `save_points` -> [0] is not an unsigned integer",
      }
    }, "value_errors_1.json");

    assert_parse({
      {}, {
        "invalid `grid_state` -> fill value must be <= 255",
        "invalid `rules` -> [0].replacement is not an unsigned integer",
        "invalid `save_points` -> [2] is not an unsigned integer",
      }
    }, "value_errors_2.json");

    assert_parse({
      {}, {
        "invalid `rules` -> [0].replacement is > 255",
      }
    }, "value_errors_3.json");

    assert_parse({
      {}, {
        "invalid `rules` -> [0].turn is not one of L|N|R",
      }
    }, "value_errors_4.json");

    assert_parse({
      {}, {
        "invalid `rules` -> [0].turn is not one of L|N|R"
      }
    }, "value_errors_5.json");

    //   {
    //     uint8_t grid[50];
    //     seqgen::populate(grid, "0{50}");

    //     assert_parse({
    //       {
    //         0, // generations
    //         step_result::NIL,
    //         5, // grid_width
    //         10, // grid_height
    //         -1, // ant_col
    //         11, // ant_row
    //         orientation::WEST,
    //         nullptr, // grid
    //         generate_rules({
    //           // 0->1->2
    //           { 0, {1, turn_direction::LEFT} },
    //           { 1, {2, turn_direction::LEFT} },
    //         }),
    //         0, // save_interval
    //         8, // num_save_points
    //         { // save_points
    //           0, 0,
    //           1, 1, 1,
    //           2, 2, 2,
    //         },
    //       }, {}
    //     }, "_.json");
    //   }
    // }
  }

  ntest::generate_report("report");

  return 0;
}
