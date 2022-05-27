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

std::string file_as_string(char const *const fpathname) {
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

  {
    std::vector<Assertion> assertions{};

    { // https://mathworld.wolfram.com/images/gifs/langant.gif
      char const *const simName = "RL";
      uint_fast16_t const gridWidth = 7, gridHeight = 8;
      Simulation sim(
        simName,
        gridWidth, gridHeight, 1,
        2, 2, AO_NORTH,
        std::array<Rule, 256>{{
          Rule(1, TD_RIGHT),
          Rule(0, TD_LEFT),
        }}
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
    }

    run_suite("Simulation::step_once", assertions);
  }

  {
    std::vector<Assertion> assertions{};

    auto const saveSimAndAssertResultingFiles = [&assertions](
      char const *const simName,
      Simulation const &sim,
      uint8_t const *expectedGrid
    ){
      std::string const
        fsimOutPathname = std::string("tests/out/") + simName + ".sim",
        fsimInPathname = std::string("tests/expected/") + simName + ".sim",
        fpgmPathname = std::string("tests/out/") + simName + ".pgm";

      { // generate files
        std::ofstream
          fsimOut = open_and_assert_fstream<std::ofstream>(fsimOutPathname.c_str()),
          fpgmOut = open_and_assert_fstream<std::ofstream>(fpgmPathname.c_str());
        sim.save(fsimOut, fpgmOut, fpgmPathname);
      }

      // verify sim file
      assertions.emplace_back(
        simName,
        file_as_string(fsimOutPathname.c_str())
          == file_as_string(fsimInPathname.c_str())
      );

      if (expectedGrid == nullptr) {
        return;
      }

      std::ifstream fpgmIn =
        open_and_assert_fstream<std::ifstream>(fpgmPathname.c_str());
      pgm8::Image img(fpgmIn);

      // verify PGM file
      assertions.emplace_back(
        simName,
        (
          sim.grid_width() == img.width() &&
          sim.grid_height() == img.height() &&
          // check that pixels match
          arr2d::cmp(img.pixels(), expectedGrid, img.width(), img.height())
        )
      );
    };

    {
      char const *const simName = "LNR";
      Simulation sim(
        simName,
        100, 100, 0,
        50, 50, AO_NORTH,
        std::array<Rule, 256>{{
          Rule(1, TD_LEFT),
          Rule(2, TD_NONE),
          Rule(0, TD_RIGHT),
        }}
      );
      saveSimAndAssertResultingFiles(simName, sim, nullptr);
    }

    { // https://mathworld.wolfram.com/images/gifs/langant.gif
      char const *const simName = "RL";
      uint_fast16_t const gridWidth = 7, gridHeight = 8;
      Simulation sim(
        simName,
        gridWidth, gridHeight, 1,
        2, 2, AO_NORTH,
        std::array<Rule, 256>{{
          Rule(1, TD_RIGHT),
          Rule(0, TD_LEFT),
        }}
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
      saveSimAndAssertResultingFiles(simName, sim, nullptr);
    }

    run_suite("Simulation::save", assertions);
  }

  return 0;
}
