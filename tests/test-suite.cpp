#include <fstream>
#include <vector>
#include <sstream>
#include "../cpp-lib/includes/lengthof.hpp"
#include "../cpp-lib/includes/arr2d.hpp"
#include "../cpp-lib/includes/pgm8.hpp"
#include "../cpp-lib/includes/test.hpp"
#include "../src/ant-simulator-core.hpp"

using
  test::Assertion, test::run_suite,
  term::printf_colored, term::ColorText,
  asc::Simulation, asc::Rule;

#define LOOP_N(n, stmt) \
for (size_t i = 1; i <= n; ++i) stmt;

template<typename FstreamT>
void assert_file(FstreamT const *file, char const *const fpathname) {
  if (!file->is_open()) {
    printf_colored(
      ColorText::RED,
      "failed to open file `%s`",
      fpathname
    );
    exit(-1);
  }
}

std::string file_as_str(char const *const fpathname) {
  std::ifstream file(fpathname);
  assert_file(&file, fpathname);
  std::stringstream ss{};
  ss << file.rdbuf();
  return ss.str();
}

template<typename FstreamT>
FstreamT open_and_assert_fstream(char const *const fpathname) {
  FstreamT file(fpathname);
  assert_file<FstreamT>(&file, fpathname);
  return file;
}

int main(int const argc, char const *const *const argv) {
  if (argc < 2) {
    printf_colored(ColorText::YELLOW, "usage: <out_path>");
    return 1;
  }

  term::set_color_text_default(ColorText::DEFAULT);

  std::ofstream result;
  {
    std::string const pathname = std::string(argv[1]) + "/assertions.txt";
    result = std::ofstream(pathname);
    assert_file(&result, pathname.c_str());
    test::set_ofstream(&result);
  }

  std::vector<Assertion> assertions{};

  #pragma region Simulation validation
  {
    std::vector<std::string> errors{};

    auto const printErrors = [&errors]() {
      for (size_t i = 0; i < errors.size(); ++i) {
        term::printf_colored(
          ColorText::BLUE,
          "errors[%zu]: %s", i, (errors[i] + "\n").c_str()
        );
      }
    };

    Simulation::validate_grid_width(errors, 1);
    Simulation::validate_grid_width(errors, 10);
    Simulation::validate_grid_width(errors, 10000);
    Simulation::validate_grid_width(errors, UINT16_MAX);
    assertions.emplace_back("validate_grid_width with valid input", errors.empty());

    errors.clear();

    Simulation::validate_grid_width(errors, 0);
    assertions.emplace_back(
      "validate_grid_width(0)",
      errors[0] == "gridWidth (0) not in range [1, 65535]"
    );
    Simulation::validate_grid_width(errors, static_cast<uint_fast16_t>(UINT16_MAX + 1));
    assertions.emplace_back(
      "validate_grid_width(UINT16_MAX + 1)",
      errors[1] == "gridWidth (65536) not in range [1, 65535]"
    );

    // printErrors();
    errors.clear();

    Simulation::validate_grid_height(errors, 1);
    Simulation::validate_grid_height(errors, 10);
    Simulation::validate_grid_height(errors, 10000);
    Simulation::validate_grid_height(errors, UINT16_MAX);
    assertions.emplace_back("validate_grid_height with valid input", errors.empty());

    errors.clear();

    Simulation::validate_grid_height(errors, 0);
    assertions.emplace_back(
      "validate_grid_height(0)",
      errors[0] == "gridHeight (0) not in range [1, 65535]"
    );
    Simulation::validate_grid_height(errors, static_cast<uint_fast16_t>(UINT16_MAX + 1));
    assertions.emplace_back(
      "validate_grid_height(UINT16_MAX + 1)",
      errors[1] == "gridHeight (65536) not in range [1, 65535]"
    );

    // printErrors();
    errors.clear();

    Simulation::validate_ant_col(errors, 0,              10);
    Simulation::validate_ant_col(errors, 10,             11);
    Simulation::validate_ant_col(errors, 10000,          UINT16_MAX);
    Simulation::validate_ant_col(errors, UINT16_MAX - 1, UINT16_MAX);
    assertions.emplace_back("validate_ant_col with valid input", errors.empty());

    errors.clear();

    Simulation::validate_ant_col(errors, 10, 10);
    assertions.emplace_back(
      "validate_ant_col(10, 10)",
      errors[0] == "antCol (10) not within gridWidth (10)"
    );
    Simulation::validate_ant_col(errors, 100, 10);
    assertions.emplace_back(
      "validate_ant_col(100, 10)",
      errors[1] == "antCol (100) not within gridWidth (10)"
    );

    // printErrors();
    errors.clear();

    Simulation::validate_ant_row(errors, 0,              10);
    Simulation::validate_ant_row(errors, 10,             11);
    Simulation::validate_ant_row(errors, 10000,          UINT16_MAX);
    Simulation::validate_ant_row(errors, UINT16_MAX - 1, UINT16_MAX);
    assertions.emplace_back("validate_ant_row with valid input", errors.empty());

    errors.clear();

    Simulation::validate_ant_row(errors, 10, 10);
    assertions.emplace_back(
      "validate_ant_row(10, 10)",
      errors[0] == "antRow (10) not within gridHeight (10)"
    );
    Simulation::validate_ant_row(errors, 100, 10);
    assertions.emplace_back(
      "validate_ant_row(100, 10)",
      errors[1] == "antRow (100) not within gridHeight (10)"
    );

    // printErrors();
    errors.clear();

    Rule const blankRule{};
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      // 1->0
      // ^__|
      /* 0 */ Rule(1, TD_LEFT),
      /* 1 */ Rule(0, TD_RIGHT),
    });
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      // 1->3->4
      // ^_____|
      /* 0 */ blankRule,
      /* 1 */ Rule(3, TD_LEFT),
      /* 2 */ blankRule,
      /* 3 */ Rule(4, TD_RIGHT),
      /* 4 */ Rule(1, TD_NONE),
    });
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      // 1->5->3
      // ^_____|
      /* 0 */ blankRule,
      /* 1 */ Rule(5, TD_LEFT),
      /* 2 */ blankRule,
      /* 3 */ Rule(1, TD_RIGHT),
      /* 4 */ blankRule,
      /* 5 */ Rule(3, TD_NONE),
    });
    assertions.emplace_back("validate_rules with valid input", errors.empty());

    errors.clear();

    Simulation::validate_rules(errors, std::array<Rule, 256>{});
    assertions.emplace_back(
      "validate_rules with 0 rules defined",
      errors[0] == "fewer than 2 rules defined"
    );
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      /* 0 */ Rule(1, TD_LEFT),
    });
    assertions.emplace_back(
      "validate_rules with 1 rule defined",
      errors[1] == "fewer than 2 rules defined"
    );
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      /* 0 */ Rule(1, TD_LEFT),
      /* 1 */ Rule(2, TD_RIGHT),
    });
    assertions.emplace_back(
      "0->1->2",
      errors[2] == "rules don't form a closed chain"
    );
    Simulation::validate_rules(errors, std::array<Rule, 256>{
      // 0->1
      // ^--|
      //    2
      /* 0 */ Rule(1, TD_LEFT),
      /* 1 */ Rule(0, TD_RIGHT),
      /* 2 */ Rule(0, TD_NONE),
    });
    assertions.emplace_back(
      "0->1->0, 2->0",
      errors[3] == "rules don't form a closed chain"
    );

    // printErrors();
    errors.clear();

    run_suite("Simulation validation", assertions);
  }
  #pragma endregion

  assertions.clear();

  #pragma region Simulation::step_once
  { // https://mathworld.wolfram.com/images/gifs/langant.gif
    char const *const simName = "RL";
    uint_fast16_t const gridWidth = 7, gridHeight = 8;
    Simulation sim(
      simName, 0,
      gridWidth, gridHeight, 1,
      2, 2, AO_NORTH,
      std::array<Rule, 256>{{
        Rule(1, TD_RIGHT),
        Rule(0, TD_LEFT),
      }},
      std::vector<uint_fast64_t>{},
      std::vector<uint_fast64_t>{}
    );

    LOOP_N(50, sim.step_once());

    uint8_t const expectedGrid[gridWidth * gridHeight] {
      1,1,1,1,1,1,1,
      1,1,0,0,1,1,1,
      1,0,1,1,0,1,1,
      1,0,1,1,1,0,1,
      1,1,0,1,1,0,1,
      1,0,1,1,0,1,1,
      1,1,0,0,1,1,1,
      1,1,1,1,1,1,1,
    };

    assertions.emplace_back(
      simName,
      arr2d::cmp(
        expectedGrid, sim.grid(), gridWidth, gridHeight
      ) &&
      sim.ant_col() == 1 &&
      sim.ant_row() == 3 &&
      sim.ant_orientation() == AO_NORTH
    );

    run_suite("Simulation::step_once", assertions);
  }
  #pragma endregion

  assertions.clear();

  #pragma region Simulation::save and snapshots
  asc::set_save_path(argv[1]);

  {
    // gif: https://mathworld.wolfram.com/images/gifs/langant.gif
    // split images: https://ezgif.com/split/ezgif-1-d048506a68.gif
    char const *const simName = "RL";
    uint_fast16_t const gridWidth = 7, gridHeight = 8;

    Simulation sim(
      simName, 0,
      gridWidth, gridHeight, 1,
      2, 2, AO_NORTH,
      std::array<Rule, 256>{{
        Rule(1, TD_RIGHT),
        Rule(0, TD_LEFT),
      }},
      std::vector<uint_fast64_t>{3, 50},
      std::vector<uint_fast64_t>{16}
    );

    LOOP_N(50, sim.step_once());

    size_t const expectedSnapshotCount = 5;
    std::array<uint_fast64_t, expectedSnapshotCount> const expectedSnapshots {
      3, 16, 32, 48, 50
    };

    for (size_t i = 0; i < expectedSnapshotCount; ++i) {
      uint_fast64_t const snap = expectedSnapshots[i];

      std::string const assertionName = ([simName, snap]() {
        std::stringstream ss{};
        ss << simName << '(' << std::to_string(snap) << ')';
        return ss.str();
      })();

      std::string const
        fsimPathnameOut = "tests/out/" + assertionName + ".sim",
        fsimPathnameExpected = "tests/expected/" + assertionName + ".sim",
        fpgmPathnameOut = "tests/out/" + assertionName + ".pgm",
        fpgmPathnameExpected = "tests/expected/" + assertionName + ".pgm";

      // verify sim file
      assertions.emplace_back(
        std::string(assertionName + " sim file"),
        file_as_str(fsimPathnameOut.c_str()) ==
          file_as_str(fsimPathnameExpected.c_str())
      );

      auto fpgmIn = open_and_assert_fstream<std::ifstream>(fpgmPathnameOut.c_str());
      pgm8::Image img(fpgmIn);

      // verify PGM file
      assertions.emplace_back(
        std::string(assertionName + " PGM file"),
        file_as_str(fpgmPathnameOut.c_str()) ==
          file_as_str(fpgmPathnameExpected.c_str())
      );
    }
  }

  run_suite("Simulation::save and snapshots", assertions);
  #pragma endregion

  assertions.clear();

  return 0;
}
