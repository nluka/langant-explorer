#include <thread>

#include "ntest.hpp"
#include "simulation.hpp"
#include "core_source_libs.hpp"
#include "program_options.hpp"

namespace fs = std::filesystem;
using json_t = nlohmann::json;
using util::errors_t;
using util::lengthof;

simulation::rules_t generate_rules(
  std::vector<std::pair<
    u64, // shade
    simulation::rule
  >> const &scheme)
{
  simulation::rules_t rules{};

  for (auto const &[shade, rule] : scheme)
    rules[shade] = rule;

  return rules;
}

i32 main()
{
  using namespace term;

  printf(FG_BLUE, "std::filesystem::current_path() = %s\n", fs::current_path().string().c_str());

  ntest::config::set_max_arr_preview_len(1);
  ntest::config::set_max_str_preview_len(10);

  {
    auto const res = ntest::init();
    printf(FG_BLUE, "%zu residual files removed", res.num_files_removed);
    if (res.num_files_failed_to_remove > 0)
      printf(FG_YELLOW, " (%zu files failed to be removed)", res.num_files_failed_to_remove);
    printf("\n");
  }

  #if 1 // simulation::parse_state
  {
    auto const assert_parse = [](
      simulation::state const &expected_state,
      errors_t const &expected_errors,
      char const *const state_path,
      std::source_location const loc = std::source_location::current())
    {
      std::string const json_str = util::extract_txt_file_contents(
        std::string("testing/parse/") + state_path,
        true);

      errors_t actual_errors{};

      simulation::state const actual_state = simulation::parse_state(
        json_str,
        fs::path("testing/parse/"),
        actual_errors);

      if (!expected_errors.empty()) {
        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      } else {
        ntest::assert_uint64(expected_state.start_generation, actual_state.start_generation, loc);
        ntest::assert_uint64(expected_state.generation, actual_state.generation, loc);
        ntest::assert_int32(expected_state.grid_width, actual_state.grid_width, loc);
        ntest::assert_int32(expected_state.grid_height, actual_state.grid_height, loc);
        ntest::assert_int32(expected_state.ant_col, actual_state.ant_col, loc);
        ntest::assert_int32(expected_state.ant_row, actual_state.ant_row, loc);
        ntest::assert_int8(expected_state.ant_orientation, actual_state.ant_orientation, loc);
        ntest::assert_stdarr(expected_state.rules, actual_state.rules, loc);
        ntest::assert_arr(
          expected_state.grid,
          expected_state.grid == nullptr ? 0 : actual_state.num_pixels(),
          actual_state.grid,
          actual_state.grid == nullptr ? 0 : actual_state.num_pixels(),
          loc);
      }
    };

    assert_parse({/* state */}, {
      "`generation` not set",
      "`last_step_result` not set",
      "`grid_width` not set",
      "`grid_height` not set",
      "`grid_state` not set",
      "`ant_col` not set",
      "`ant_row` not set",
      "`ant_orientation` not set",
      "`rules` not set",
    }, "occurence_errors.json");

    assert_parse({/* state */}, {
      // errors:
      "bad generation, type must be number, but is string",
      "bad grid_width, type must be number, but is object",
      "bad grid_height, type must be number, but is array",
      "bad ant_col, type must be number, but is null",
      "bad ant_row, type must be number, but is string",
      "bad last_step_result, type must be string, but is number",
      "bad ant_orientation, type must be string, but is number",
      "bad rules, type must be array, but is object",
      "bad grid_state, type must be string, but is boolean",
    }, "type_errors_0.json");

    assert_parse({/* state */}, {
      // errors:
      "bad generation, type must be number, but is null",
      "bad grid_width, type must be number, but is array",
      "bad grid_height, type must be number, but is object",
      "bad ant_col, type must be number, but is string",
      "bad ant_row, type must be number, but is null",
      "bad last_step_result, type must be string, but is boolean",
      "bad ant_orientation, type must be string, but is boolean",
      "bad rules, not an array of objects",
      "bad grid_state, type must be string, but is number",
    }, "type_errors_1.json");

    assert_parse({/* state */}, {
      // errors:
      "bad generation, not an unsigned integer",
      "bad grid_width, not an unsigned integer",
      "bad grid_height, not an unsigned integer",
      "bad ant_col, not an unsigned integer",
      "bad ant_row, not an unsigned integer",
      "bad last_step_result, not one of nil|success|hit_edge",
      "bad ant_orientation, not one of N|E|S|W",
      "bad rules, [0].on is not an unsigned integer",
      "bad grid_state, cannot be blank",
    }, "value_errors_0.json");

    assert_parse({/* state */}, {
      // errors:
      "bad grid_width, cannot be > 65535",
      "bad grid_height, cannot be > 65535",
      "bad ant_col, cannot be > 65535",
      "bad ant_row, cannot be > 65535",
      "bad last_step_result, not one of nil|success|hit_edge",
      "bad ant_orientation, not one of N|E|S|W",
      "bad rules, [0].on is > 255",
      "bad grid_state, fill shade cannot be negative",
    }, "value_errors_1.json");

    assert_parse({/* state */}, {
      // errors:
      "bad ant_col, not in grid x-axis [0, 65535)",
      "bad ant_row, not in grid y-axis [0, 65535)",
      "bad rules, [0].replace_with is not an unsigned integer",
      "bad grid_state, fill shade must be <= 255",
    }, "value_errors_2.json");

    assert_parse({/* state */}, {
      // errors:
      "bad grid_width, must be in range [1, 65535]",
      "bad grid_height, must be in range [1, 65535]",
      "bad rules, [0].replace_with is > 255",
      "bad grid_state, file \"non_existent_file\" does not exist",
    }, "value_errors_3.json");

    assert_parse({/* state */}, {
      // errors:
      "bad rules, [0].turn not recognized",
      "bad grid_state, file \"fake_file\" does not exist",
    }, "value_errors_4.json");

    assert_parse({/* state */}, {
      // errors:
      "bad rules, [0].turn is empty"
    }, "value_errors_5.json");

    assert_parse({/* state */}, {
      // errors:
      "bad rules, fewer than 2 defined"
    }, "value_errors_6.json");

    assert_parse({/* state */}, {
      // errors:
      "bad rules, don't form a closed chain",
    }, "value_errors_7.json");

    assert_parse({/* state */}, {
      // errors:
      "bad grid_state, fill shade has no governing rule",
    }, "value_errors_8.json");

    assert_parse({/* state */}, {
      // errors:
      "bad rules, don't form a closed chain"
    }, "value_errors_9.json");

    {
      u8 grid[5 * 6];
      std::fill_n(grid, 5 * 6, u8(0));

      simulation::state expected_state{};
      expected_state.grid_width = 5;
      expected_state.grid_height = 6;
      expected_state.ant_col = 2;
      expected_state.ant_row = 3;
      expected_state.grid = grid;
      expected_state.ant_orientation = simulation::orientation::WEST;
      expected_state.rules = generate_rules({
        // 0->1->2
        { 0, { 1, simulation::turn_direction::LEFT } },
        { 1, { 0, simulation::turn_direction::RIGHT } },
      }),

      assert_parse(expected_state, {/* no errors */}, "good_fill.json");
    }

    {
      u8 grid[5 * 5] {
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
        1, 0, 1, 0, 1,
        0, 1, 0, 1, 0,
      };

      simulation::state expected_state{};
      expected_state.start_generation = 1000;
      expected_state.generation = 1000;
      expected_state.last_step_res = simulation::step_result::HIT_EDGE;
      expected_state.grid_width = 5;
      expected_state.grid_height = 5;
      expected_state.ant_col = 2;
      expected_state.ant_row = 2;
      expected_state.grid = grid;
      expected_state.ant_orientation = simulation::orientation::WEST;
      expected_state.rules = generate_rules({
        // 0->1->2
        { 0, { 1, simulation::turn_direction::LEFT } },
        { 1, { 2, simulation::turn_direction::NO_CHANGE } },
        { 2, { 0, simulation::turn_direction::RIGHT } },
      }),

      assert_parse(expected_state, {/* no errors */}, "good_img.json");
    }
  }
  #endif // simulation::parse_state

  #if 1 // util::parse_json_array_u64
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
  #endif // util::parse_json_array_u64

  #if 1 // simulation::run
  {
    auto const assert_save_point = [](
      char const *const exp_json_name_cstr,
      char const *const exp_img_name_cstr,
      char const *const act_json_name_cstr,
      std::source_location const loc = std::source_location::current())
    {
      std::string const base = "testing/run/";

      std::string const
        exp_json_pathname = base + exp_json_name_cstr,
        act_json_pathname = base + act_json_name_cstr;

      std::string const
        exp_json_str = util::extract_txt_file_contents(exp_json_pathname, false),
        act_json_str = util::extract_txt_file_contents(act_json_pathname, false);

      json_t const
        exp_json = json_t::parse(exp_json_str),
        act_json = json_t::parse(act_json_str);

      using array_t = json_t::array_t;

      b8 match =
        exp_json["generation"]           == act_json["generation"]       &&
        exp_json["last_step_result"]     == act_json["last_step_result"] &&
        exp_json["grid_width"]           == act_json["grid_width"]       &&
        exp_json["grid_height"]          == act_json["grid_height"]      &&
        exp_json["ant_col"]              == act_json["ant_col"]          &&
        exp_json["ant_row"]              == act_json["ant_row"]          &&
        exp_json["ant_orientation"]      == act_json["ant_orientation"]  &&
        exp_json["rules"].get<array_t>() == act_json["rules"].get<array_t>();

      std::string const act_img_pathname = base + act_json["grid_state"].get<std::string>();

      std::string const
        expected_img = util::extract_txt_file_contents(base + exp_img_name_cstr, true),
        actual_img = util::extract_txt_file_contents(act_img_pathname, true);

      match = match && (expected_img == actual_img);

      ntest::assert_bool(true, match, loc);
    };

    fs::path const save_dir = "testing/run/";
    errors_t errors{};

    // starting from generation 0, plain image
    {
      std::string const json_str = util::extract_txt_file_contents("testing/run/RL-init.json", false);
      simulation::state state = simulation::parse_state(json_str, save_dir, errors);
      assert(errors.empty());

      {
        auto const to_delete = fregex::find(
          save_dir.string().c_str(),
          "RL_plain\\.actual.*",
          fregex::entry_type::regular_file);

        for (auto const &file : to_delete) {
          fs::remove_all(file);
        }
      }

      simulation::run(
        state,
        "RL_plain.actual",
        50, // generation_limit
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::PLAIN,
        save_dir,
        true, // save_final_state
        false, // create_logs
        false // save_image_only
      );

      assert_save_point("RL_plain.expect(3).json",  "RL_plain.expect(3).pgm",  "RL_plain.actual(3).json");
      assert_save_point("RL_plain.expect(16).json", "RL_plain.expect(16).pgm", "RL_plain.actual(16).json");
      assert_save_point("RL_plain.expect(32).json", "RL_plain.expect(32).pgm", "RL_plain.actual(32).json");
      assert_save_point("RL_plain.expect(48).json", "RL_plain.expect(48).pgm", "RL_plain.actual(48).json");
      assert_save_point("RL_plain.expect(50).json", "RL_plain.expect(50).pgm", "RL_plain.actual(50).json");
    }

    // starting from generation 16, plain image
    {
      std::string const json_str = util::extract_txt_file_contents("testing/run/RL_plain.expect(16).json", false);
      simulation::state state = simulation::parse_state(json_str, save_dir, errors);
      assert(errors.empty());

      {
        auto const to_delete = fregex::find(
          save_dir.string().c_str(),
          "RL_plain_from16\\.actual.*",
          fregex::entry_type::regular_file);

        for (auto const &file : to_delete) {
          fs::remove_all(file);
        }
      }

      simulation::run(
        state,
        "RL_plain_from16.actual",
        50, // generation_limit
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::PLAIN,
        save_dir,
        true, // save_final_state
        false, // create_logs
        false // save_image_only
      );

      assert_save_point("RL_plain.expect(32).json", "RL_plain.expect(32).pgm", "RL_plain_from16.actual(32).json");
      assert_save_point("RL_plain.expect(48).json", "RL_plain.expect(48).pgm", "RL_plain_from16.actual(48).json");
      assert_save_point("RL_plain.expect(50).json", "RL_plain.expect(50).pgm", "RL_plain_from16.actual(50).json");
    }

    // starting from generation 0, raw image
    {
      std::string const json_str = util::extract_txt_file_contents("testing/run/RL-init.json", false);
      simulation::state state = simulation::parse_state(json_str, save_dir, errors);
      assert(errors.empty());

      {
        auto const to_delete = fregex::find(
          save_dir.string().c_str(),
          "RL_raw\\.actual.*",
          fregex::entry_type::regular_file);

        for (auto const &file : to_delete) {
          fs::remove_all(file);
        }
      }

      simulation::run(
        state,
        "RL_raw.actual",
        50, // generation_limit
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::RAW,
        save_dir,
        true, // save_final_state
        false, // create_logs
        false // save_image_only
      );

      assert_save_point("RL_raw.expect(3).json",  "RL_raw.expect(3).pgm",  "RL_raw.actual(3).json");
      assert_save_point("RL_raw.expect(16).json", "RL_raw.expect(16).pgm", "RL_raw.actual(16).json");
      assert_save_point("RL_raw.expect(32).json", "RL_raw.expect(32).pgm", "RL_raw.actual(32).json");
      assert_save_point("RL_raw.expect(48).json", "RL_raw.expect(48).pgm", "RL_raw.actual(48).json");
      assert_save_point("RL_raw.expect(50).json", "RL_raw.expect(50).pgm", "RL_raw.actual(50).json");
    }

    // starting from generation 16, raw image
    {
      std::string const json_str = util::extract_txt_file_contents("testing/run/RL_raw.expect(16).json", false);
      simulation::state state = simulation::parse_state(json_str, save_dir, errors);
      assert(errors.empty());

      {
        auto const to_delete = fregex::find(
          save_dir.string().c_str(),
          "RL_raw_from16\\.actual.*",
          fregex::entry_type::regular_file);

        for (auto const &file : to_delete) {
          fs::remove_all(file);
        }
      }

      simulation::run(
        state,
        "RL_raw_from16.actual",
        50, // generation_limit
        { 3, 50 }, // save_points
        16, // save_interval
        pgm8::format::RAW,
        save_dir,
        true, // save_final_state
        false, // create_logs
        false // save_image_only
      );

      assert_save_point("RL_raw.expect(32).json", "RL_raw.expect(32).pgm", "RL_raw_from16.actual(32).json");
      assert_save_point("RL_raw.expect(48).json", "RL_raw.expect(48).pgm", "RL_raw_from16.actual(48).json");
      assert_save_point("RL_raw.expect(50).json", "RL_raw.expect(50).pgm", "RL_raw_from16.actual(50).json");
    }
  }
  #endif // simulation::run

  #if 1 // po::parse_simulate_one_options
  {
    auto const assert_parse = [](
      u64 const argc,
      char const *const *const argv,
      po::simulate_one_options const &expected_options,
      errors_t &expected_errors,
      std::source_location const loc = std::source_location::current())
    {
      errors_t actual_errors{};
      po::simulate_one_options actual_options{};

      po::parse_simulate_one_options(static_cast<int>(argc), argv, actual_options, actual_errors);

      std::sort(expected_errors.begin(), expected_errors.end(), util::ascending_order<std::string>());
      std::sort(actual_errors.begin(), actual_errors.end(), util::ascending_order<std::string>());

      if (!expected_errors.empty())
        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      else {
        auto const str_opts = ntest::default_str_opts();

        ntest::assert_stdstr(expected_options.name, actual_options.name, str_opts, loc);
        ntest::assert_stdstr(expected_options.state_file_path, actual_options.state_file_path, str_opts, loc);
        ntest::assert_stdstr(expected_options.log_file_path, actual_options.log_file_path, str_opts, loc);

        ntest::assert_stdstr(expected_options.sim.save_path, actual_options.sim.save_path, str_opts, loc);
        ntest::assert_stdvec(expected_options.sim.save_points, actual_options.sim.save_points, loc);
        ntest::assert_uint64(expected_options.sim.generation_limit, actual_options.sim.generation_limit, loc);
        ntest::assert_uint8((u8)expected_options.sim.image_format, (u8)actual_options.sim.image_format, loc);
        ntest::assert_bool(expected_options.sim.save_final_state, actual_options.sim.save_final_state, loc);
        ntest::assert_bool(expected_options.sim.create_logs, actual_options.sim.create_logs, loc);
        ntest::assert_bool(expected_options.sim.save_image_only, actual_options.sim.save_image_only, loc);
      }
    };

    {
      char const *const argv[] {
        "simulate_one",
      };

      errors_t expected_errors {
        "-S [ --state_file_path ] required",
        "-g [ --generation_limit ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-g", "0",
        "-p", "[]", // empty, -o NOT required
      };

      errors_t expected_errors {
        "-S [ --state_file_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-g", "0",
        "-s", // therefore -o is required
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
        "-S [ --state_file_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-S", "testing/valid_dir",
        "-g", "0",
        "-o", "testing/valid_dir",
      };

      errors_t expected_errors {
        "-S [ --state_file_path ] must be a regular file",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-S", "testing/valid_dir/valid_regular_file",
        "-g", "0",
        "-f", "unknown",
        "-l", // therefore -L is required
        "-v", "10", // therefore -o is required
      };

      errors_t expected_errors {
        "-L [ --log_file_path ] required",
        "-o [ --save_path ] required",
        "-f [ --image_format ] must be one of raw|plain",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-S", "testing/valid_dir/valid_regular_file",
        "-g", "0",
        "-p", "[1,2,3]", // therefore -o is required
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-S", "testing/valid_dir/valid_regular_file",
        "-g", "0",
        "-s", // therefore -o is required
        "-p", "1,2,3", // missing []
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
        "-p [ --save_points ] must be a JSON array",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-S", "testing/valid_dir/valid_regular_file",
        "-g", "0",
      };

      po::simulate_one_options const expected_options {
        "", // name
        "testing/valid_dir/valid_regular_file", // state_file_path
        "", // log_file_path
        {
          "", // save_path
          {}, // save_points
          0, // generation_limit
          0, // save_interval
          pgm8::format::RAW, // image_format
          false, // save_final_state
          false, // create_logs
          false, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-N", "name",
        "-S", "testing/valid_dir/valid_regular_file",
        "-L", "testing/valid_dir/log.txt",
        "-o", "testing/valid_dir",
        "-p", "[1,20,300]",
        "-g", "0",
        "-v", "10",
        "-f", "raw",
        "-s",
        "-l",
        "-y",
      };

      po::simulate_one_options const expected_options {
        "name", // name
        "testing/valid_dir/valid_regular_file", // state_file_path
        "testing/valid_dir/log.txt", // log_file_path
        {
          "testing/valid_dir", // save_path
          { 1, 20, 300 }, // save_points
          0, // generation_limit
          10, // save_interval
          pgm8::format::RAW, // image_format
          true, // save_final_state
          true, // create_logs
          true, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_one",
        "-N", "name2",
        "-S", "testing/valid_dir/valid_regular_file",
        "-L", "testing/valid_dir/log.txt",
        "-o", "testing/valid_dir",
        "-p", "[]",
        "-g", "1000",
        "-v", "42",
        "-f", "plain",
      };

      po::simulate_one_options const expected_options {
        "name2", // name
        "testing/valid_dir/valid_regular_file", // state_file_path
        "testing/valid_dir/log.txt", // log_file_path
        {
          "testing/valid_dir", // save_path
          {}, // save_points
          1000, // generation_limit
          42, // save_interval
          pgm8::format::PLAIN, // image_format
          false, // save_final_state
          false, // create_logs
          false, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }
  }
  #endif // po::parse_simulate_one_options

  #if 1 // po::parse_simulate_many_options
  {
    auto const assert_parse = [](
      u64 const argc,
      char const *const *const argv,
      po::simulate_many_options const &expected_options,
      errors_t &expected_errors,
      std::source_location const loc = std::source_location::current())
    {
      errors_t actual_errors{};
      po::simulate_many_options actual_options{};

      po::parse_simulate_many_options(static_cast<int>(argc), argv, actual_options, actual_errors);

      std::sort(expected_errors.begin(), expected_errors.end(), util::ascending_order<std::string>());
      std::sort(actual_errors.begin(), actual_errors.end(), util::ascending_order<std::string>());

      if (!expected_errors.empty())
        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      else {
        assert(actual_errors.empty());

        auto const str_opts = ntest::default_str_opts();

        ntest::assert_stdstr(expected_options.state_dir_path, actual_options.state_dir_path, str_opts, loc);
        ntest::assert_stdstr(expected_options.log_file_path, actual_options.log_file_path, str_opts, loc);
        ntest::assert_uint64(expected_options.num_threads, actual_options.num_threads, loc);

        ntest::assert_stdstr(expected_options.sim.save_path, actual_options.sim.save_path, str_opts, loc);
        ntest::assert_stdvec(expected_options.sim.save_points, actual_options.sim.save_points, loc);
        ntest::assert_uint64(expected_options.sim.generation_limit, actual_options.sim.generation_limit, loc);
        ntest::assert_uint8((u8)expected_options.sim.image_format, (u8)actual_options.sim.image_format, loc);
        ntest::assert_bool(expected_options.sim.save_final_state, actual_options.sim.save_final_state, loc);
        ntest::assert_bool(expected_options.sim.create_logs, actual_options.sim.create_logs, loc);
        ntest::assert_bool(expected_options.sim.save_image_only, actual_options.sim.save_image_only, loc);
      }
    };

    {
      char const *const argv[] {
        "simulate_many",
      };

      errors_t expected_errors {
        "-S [ --state_dir_path ] required",
        "-g [ --generation_limit ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-g", "0",
        "-p", "[]", // empty, -o NOT required
      };

      errors_t expected_errors {
        "-S [ --state_dir_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-g", "0",
        "-s", // therefore -o is required
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
        "-S [ --state_dir_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-S", "testing/valid_dir/valid_regular_file",
        "-g", "0",
        "-o", "testing/valid_dir",
      };

      errors_t expected_errors {
        "-S [ --state_dir_path ] is not a directory",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-S", "testing/valid_dir",
        "-g", "0",
        "-f", "unknown",
        "-l", // therefore -L is required
        "-v", "10", // therefore -o is required
      };

      errors_t expected_errors {
        "-L [ --log_file_path ] required",
        "-o [ --save_path ] required",
        "-f [ --image_format ] must be one of raw|plain",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-S", "testing/valid_dir",
        "-g", "0",
        "-p", "[1,2,3]", // therefore -o is required
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-S", "testing/valid_dir",
        "-T", "0",
        "-g", "0",
        "-s", // therefore -o is required
        "-p", "1,2,3", // missing []
      };

      errors_t expected_errors {
        "-o [ --save_path ] required",
        "-p [ --save_points ] must be a JSON array",
        "-T [ --num_threads ] must be > 0",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-S", "testing/valid_dir",
        "-g", "0",
      };

      po::simulate_many_options const expected_options {
        "testing/valid_dir", // state_dir_path
        "", // log_file_path
        std::max(std::thread::hardware_concurrency(), u32(1)), // num_threads
        u16(50), // queue_size
        false, // log_to_stdout
        {
          "", // save_path
          {}, // save_points
          0, // generation_limit
          0, // save_interval
          pgm8::format::RAW, // image_format
          false, // save_final_state
          false, // create_logs
          false, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-T", "42",
        "-Q", "13",
        "-S", "testing/valid_dir",
        "-L", "testing/valid_dir/log.txt",
        "-o", "testing/valid_dir",
        "-p", "[1,20,300]",
        "-g", "0",
        "-v", "10",
        "-f", "raw",
        "-s",
        "-l",
        "-y",
        "-C",
      };

      po::simulate_many_options const expected_options {
        "testing/valid_dir", // state_dir_path
        "testing/valid_dir/log.txt", // log_file_path
        u32(42), // num_threads
        u16(13), // queue_size
        true, // log_to_stdout
        {
          "testing/valid_dir", // save_path
          { 1, 20, 300 }, // save_points
          0, // generation_limit
          10, // save_interval
          pgm8::format::RAW, // image_format
          true, // save_final_state
          true, // create_logs
          true, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "simulate_many",
        "-T", "1",
        "-S", "testing/valid_dir",
        "-L", "testing/valid_dir/log.txt",
        "-o", "testing/valid_dir",
        "-p", "[]",
        "-g", "1000",
        "-v", "42",
        "-f", "plain",
      };

      po::simulate_many_options const expected_options {
        "testing/valid_dir", // state_dir_path
        "testing/valid_dir/log.txt", // log_file_path
        u32(1), // num_threads
        u16(50), // queue_size
        true, // log_to_stdout
        {
          "testing/valid_dir", // save_path
          {}, // save_points
          1000, // generation_limit
          42, // save_interval
          pgm8::format::PLAIN, // image_format
          false, // save_final_state
          false, // create_logs
          false, // save_image_only
        },
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }
  }
  #endif // po::parse_simulate_many_options

  #if 1 // po::parse_make_image_options
  {
    auto const assert_parse = [](
      u64 const argc,
      char const *const *const argv,
      po::make_image_options const &expected_options,
      errors_t &expected_errors,
      std::source_location const loc = std::source_location::current())
    {
      errors_t actual_errors{};
      po::make_image_options actual_options{};

      po::parse_make_image_options(static_cast<int>(argc), argv, actual_options, actual_errors);

      std::sort(expected_errors.begin(), expected_errors.end(), util::ascending_order<std::string>());
      std::sort(actual_errors.begin(), actual_errors.end(), util::ascending_order<std::string>());

      if (!expected_errors.empty())
        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      else {
        assert(actual_errors.empty());

        auto const str_opts = ntest::default_str_opts();

        ntest::assert_stdstr(expected_options.content, actual_options.content, str_opts, loc);
        ntest::assert_stdstr(expected_options.out_file_path, actual_options.out_file_path, str_opts, loc);
        ntest::assert_bool(true, expected_options.fill_value == actual_options.fill_value, loc);
        ntest::assert_uint16(expected_options.width, actual_options.width, loc);
        ntest::assert_uint16(expected_options.height, actual_options.height, loc);
        ntest::assert_uint8((u8)expected_options.format, (u8)actual_options.format, loc);
        ntest::assert_uint8(expected_options.maxval, actual_options.maxval, loc);
      }
    };

    {
      char const *const argv[] {
        "make_image",
      };

      errors_t expected_errors {
        "-o [ --out_file_path ] required",
        "-c [ --content ] required",
        "-f [ --format ] required",
        "-w [ --width ] required",
        "-h [ --height ] required",
        "-m [ --maxval ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_image",
        "-o", "invalid_dir/file",
        "-c", "bogus",
        "-f", "bogus",
        "-w", "0",
        "-h", "0",
        "-m", "0",
      };

      errors_t expected_errors {
        "failed to open -o [ --out_file_path ]",
        "-c [ --content ] must match /^(noise)|(fill=[0-9]{1,3})$/",
        "-f [ --format ] must be one of raw|plain",
        "-w [ --width ] must be in range [1, 65535]",
        "-h [ --height ] must be in range [1, 65535]",
        "-m [ --maxval ] must be in range [1, 255]",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_image",
        "-o", "testing/valid_dir/image",
        "-c", "fill=42",
        "-f", "raw",
        "-w", "33",
        "-h", "77",
        "-m", "12",
      };

      po::make_image_options const expected_options {
        "testing/valid_dir/image", // out_file_path
        "fill=42", // content
        42, // fill_value
        33, // width
        77, // height
        pgm8::format::RAW, // format
        12, // maxval
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "make_image",
        "-o", "testing/valid_dir/image",
        "-c", "noise",
        "-f", "plain",
        "-w", "33",
        "-h", "77",
        "-m", "12",
      };

      po::make_image_options const expected_options {
        "testing/valid_dir/image", // out_file_path
        "noise", // content
        -1, // fill_value
        33, // width
        77, // height
        pgm8::format::PLAIN, // format
        12, // maxval
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }
  }
  #endif // po::parse_make_image_options

  #if 1 // po::parse_make_state_options
  {
    auto const assert_parse = [](
      u64 const argc,
      char const *const *const argv,
      po::make_states_options const &expected_options,
      errors_t &expected_errors,
      std::source_location const loc = std::source_location::current())
    {
      errors_t actual_errors{};
      po::make_states_options actual_options{};

      po::parse_make_states_options(static_cast<int>(argc), argv, actual_options, actual_errors);

      std::sort(expected_errors.begin(), expected_errors.end(), util::ascending_order<std::string>());
      std::sort(actual_errors.begin(), actual_errors.end(), util::ascending_order<std::string>());

      if (!expected_errors.empty())
        ntest::assert_stdvec(expected_errors, actual_errors, loc);
      else {
        assert(actual_errors.empty());

        auto const str_opts = ntest::default_str_opts();

        ntest::assert_stdstr(expected_options.name_mode, actual_options.name_mode, str_opts, loc);
        ntest::assert_stdstr(expected_options.grid_state, actual_options.grid_state, str_opts, loc);
        ntest::assert_stdstr(expected_options.turn_directions, actual_options.turn_directions, str_opts, loc);
        ntest::assert_stdstr(expected_options.shade_order, actual_options.shade_order, str_opts, loc);
        ntest::assert_stdstr(expected_options.ant_orientations, actual_options.ant_orientations, str_opts, loc);
        ntest::assert_stdstr(expected_options.out_dir_path, actual_options.out_dir_path, str_opts, loc);
        ntest::assert_uint64(expected_options.count, actual_options.count, loc);
        ntest::assert_uint16(expected_options.min_num_rules, actual_options.min_num_rules, loc);
        ntest::assert_uint16(expected_options.max_num_rules, actual_options.max_num_rules, loc);
        ntest::assert_int32(expected_options.grid_width, actual_options.grid_width, loc);
        ntest::assert_int32(expected_options.grid_height, actual_options.grid_height, loc);
        ntest::assert_int32(expected_options.ant_col, actual_options.ant_col, loc);
        ntest::assert_int32(expected_options.ant_row, actual_options.ant_row, loc);
      }
    };

    {
      char const *const argv[] {
        "make_states",
      };

      errors_t expected_errors {
        "-n [ --name_mode ] required",
        "-o [ --out_dir_path ] required",
        "-N [ --count ] required",
        "-w [ --grid_width ] required",
        "-h [ --grid_height ] required",
        "-x [ --ant_col ] required",
        "-y [ --ant_row ] required",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "nonsense",
        "--grid_state", "some/path",
        "--turn_directions", "nonsense",
        "--shade_order", "nonsense",
        "--ant_orientations", "nonsense",
        "--out_dir_path", "invalid_dir",
        "--count", "0",
        "--min_num_rules", "0",
        "--max_num_rules", "257",
        "--grid_width", "0",
        "--grid_height", "0",
        "--ant_col", "0",
        "--ant_row", "0",
      };

      errors_t expected_errors {
        "-o [ --out_dir_path ] does not exist, specify -c [ --create_dirs ] to create it",
        "-N [ --count ] must be > 0",
        "-O [ --ant_orientations ] must match /^[nNeEsSwW]+$/",
        "-h [ --grid_height ] must be in range [1, 65535]",
        "-n [ --name_mode ] must match /^(turndirecs)|(randwords,[1-9])|(alpha,[1-9])$/",
        "-t [ --turn_directions ] must match /^[lLnNrR]+$/",
        "-s [ --shade_order ] must be one of asc|desc|rand",
        "-w [ --grid_width ] must be in range [1, 65535]",
        "-M [ --max_num_rules ] must be in range [2, 256]",
        "-m [ --min_num_rules ] must be in range [2, 256]",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "randwords,10",
        "--out_dir_path", "testing/valid_dir",
        "--count", "1",
        "--min_num_rules", "1",
        "--max_num_rules", "257",
        "--grid_width", "1",
        "--grid_height", "1",
        "--ant_col", "1",
        "--ant_row", "1",
      };

      errors_t expected_errors {
        "-n [ --name_mode ] must match /^(turndirecs)|(randwords,[1-9])|(alpha,[1-9])$/",
        "-m [ --min_num_rules ] must be in range [2, 256]",
        "-M [ --max_num_rules ] must be in range [2, 256]",
        "-x [ --ant_col ] must be on grid x-axis [0, 1)",
        "-y [ --ant_row ] must be on grid y-axis [0, 1)",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "randwords,4",
        "--word_file_path", "testing/valid_dir",
        "--out_dir_path", "testing/valid_dir",
        "--count", "1",
        "--min_num_rules", "257",
        "--max_num_rules", "1",
        "--grid_width", "1",
        "--grid_height", "1",
        "--ant_col", "0",
        "--ant_row", "0",
      };

      errors_t expected_errors {
        "-W [ --word_file_path ] must be a regular file",
        "-m [ --min_num_rules ] must be in range [2, 256]",
        "-M [ --max_num_rules ] must be in range [2, 256]",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "randwords,4",
        "--out_dir_path", "testing/valid_dir",
        "--count", "1",
        "--min_num_rules", "10",
        "--max_num_rules", "3",
        "--grid_width", "1",
        "--grid_height", "1",
        "--ant_col", "0",
        "--ant_row", "0",
      };

      errors_t expected_errors {
        "-W [ --word_file_path ] required",
        "-M [ --max_num_rules ] must be >= -m [ --min_num_rules ]",
      };

      assert_parse(lengthof(argv), argv, {}, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "turndirecs",
        "--out_dir_path", "testing/valid_dir",
        "--count", "1",
        "--grid_width", "1",
        "--grid_height", "1",
        "--ant_col", "0",
        "--ant_row", "0",
      };

      po::make_states_options const expected_options {
        "turndirecs", // name_mode
        "fill=0", // grid_state
        "LR", // turn_directions
        "asc", // shade_order
        "NESW", // ant_orientations
        "testing/valid_dir", // out_dir_path
        "", // word_file_path
        1, // count
        2, // min_num_rules
        256, // max_num_rules
        1, // grid_width
        1, // grid_height
        0, // ant_col
        0, // ant_row
        false, // create_dirs
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "randwords,4",
        "--word_file_path", "testing/valid_dir/valid_regular_file",
        "--grid_state", "fill=42",
        "--out_dir_path", "testing/valid_dir",
        "--turn_directions", "LLLNNR",
        "--ant_orientations", "NE",
        "--shade_order", "asc",
        "--count", "42",
        "--min_num_rules", "10",
        "--max_num_rules", "30",
        "--grid_width", "12",
        "--grid_height", "22",
        "--ant_col", "11",
        "--ant_row", "21",
      };

      po::make_states_options const expected_options {
        "randwords,4", // name_mode
        "fill=42", // grid_state
        "LLLNNR", // turn_directions
        "asc", // shade_order
        "NE", // ant_orientations
        "testing/valid_dir", // out_dir_path
        "testing/valid_dir/valid_regular_file", // word_file_path
        42, // count
        10, // min_num_rules
        30, // max_num_rules
        12, // grid_width
        22, // grid_height
        11, // ant_col
        21, // ant_row
        false, // create_dirs
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "alpha,9",
        "--grid_state", "fill=42",
        "--out_dir_path", "testing/valid_dir",
        "--turn_directions", "LLLNNR",
        "--ant_orientations", "NE",
        "--shade_order", "desc",
        "--count", "42",
        "--min_num_rules", "256",
        "--max_num_rules", "256",
        "--grid_width", "12",
        "--grid_height", "22",
        "--ant_col", "11",
        "--ant_row", "21",
      };

      po::make_states_options const expected_options {
        "alpha,9", // name_mode
        "fill=42", // grid_state
        "LLLNNR", // turn_directions
        "desc", // shade_order
        "NE", // ant_orientations
        "testing/valid_dir", // out_dir_path
        "", // word_file_path
        42, // count
        256, // min_num_rules
        256, // max_num_rules
        12, // grid_width
        22, // grid_height
        11, // ant_col
        21, // ant_row
        false, // create_dirs
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }

    {
      char const *const argv[] {
        "make_states",
        "--name_mode", "turndirecs",
        "--grid_state", "fill=42",
        "--out_dir_path", "testing/valid_dir",
        "--turn_directions", "LLLNNR",
        "--ant_orientations", "NE",
        "--shade_order", "rand",
        "--count", "42",
        "--create_dirs",
        "--min_num_rules", "2",
        "--max_num_rules", "2",
        "--grid_width", "12",
        "--grid_height", "22",
        "--ant_col", "11",
        "--ant_row", "21",
      };

      po::make_states_options const expected_options {
        "turndirecs", // name_mode
        "fill=42", // grid_state
        "LLLNNR", // turn_directions
        "rand", // shade_order
        "NE", // ant_orientations
        "testing/valid_dir", // out_dir_path
        "", // word_file_path
        42, // count
        2, // min_num_rules
        2, // max_num_rules
        12, // grid_width
        22, // grid_height
        11, // ant_col
        21, // ant_row
        true, // create_dirs
      };

      errors_t expected_errors{};

      assert_parse(lengthof(argv), argv, expected_options, expected_errors);
    }
  }
  #endif // po::parse_make_state_options

  #if 1 // simulation::next_cluster
  {
    ntest::assert_uint8(1, simulation::next_cluster({}));

    ntest::assert_uint8(1, simulation::next_cluster({ "cluster0", }));

    ntest::assert_uint8(2, simulation::next_cluster({ "cluster1", }));

    ntest::assert_uint8(3, simulation::next_cluster({ "cluster1", "cluster2", }));

    ntest::assert_uint8(1, simulation::next_cluster({ "cluster2", }));

    ntest::assert_uint8(1, simulation::next_cluster({ "cluster2", "cluster3" }));

    ntest::assert_uint8(3, simulation::next_cluster({ "cluster1", "cluster2", "cluster4", }));

    {
      std::vector<fs::path> clusters{};
      clusters.reserve(255);

      std::string cluster{};

      for (u64 i = 1; i <= 255; ++i) {
        cluster = std::to_string(i);
        clusters.emplace_back("cluster" + cluster);
      }

      ntest::assert_uint8(simulation::NO_CLUSTER, simulation::next_cluster(clusters));
    }
  }
  #endif // simulation::next_cluster

  {
    char const *report_name = nullptr;

    #if ON_LINUX
    report_name = "langant-explorer-linux";
    #elif ON_WINDOWS
    report_name = "langant-explorer-windows";
    #endif

    auto const res = ntest::generate_report(report_name, [](ntest::assertion const &a, bool const passed) {
      if (!passed)
        util::print_err("failed: %s:%zu", a.loc.file_name(), a.loc.line());
    });

    if ((res.num_fails + res.num_passes) == 0) {
      printf(BG_YELLOW, "No tests defined");
    } else {
      if (res.num_fails > 0) {
        printf(BG_BRIGHT_RED | FG_BLACK,   " %zu failed ", res.num_fails);
        printf(BG_BRIGHT_GREEN | FG_BLACK, " %zu passed ", res.num_passes);
      } else
        printf(BG_BRIGHT_GREEN | FG_BLACK, " All %zu tests passed ", res.num_passes);
    }
    printf("\n");

    return 0;
  }
}
