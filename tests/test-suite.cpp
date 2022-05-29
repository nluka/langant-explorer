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

int main() {
  term::set_color_text_default(ColorText::DEFAULT);

  std::ofstream result;
  {
    std::string const pathname = "tests/out/assertions.txt";
    result = std::ofstream(pathname);
    assert_file(&result, pathname.c_str());
    test::set_ofstream(&result);
  }

  std::vector<Assertion> assertions{};

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
  asc::set_save_path("tests/out/");

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
