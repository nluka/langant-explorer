#include <fstream>
#include <vector>
#include "../cpp-lib/includes/lengthof.hpp"
#include "../cpp-lib/includes/arr2d.hpp"
#include "../cpp-lib/includes/test.hpp"
#include "../src/ant-simulator-core.hpp"

using
  test::Assertion, test::run_suite,
  term::printf_colored, term::ColorText;

#define LOOP_N(n, stmt) \
for (size_t i = 1; i <= n; ++i) stmt;

template<typename FstreamType>
void assert_file(FstreamType const *file, char const *const name) {
  if (!file->is_open()) {
    printf_colored(
      ColorText::RED,
      "failed to open file `%s`",
      name
    );
    exit(-1);
  }
}

int main() {
  term::set_color_text_default(ColorText::DEFAULT);

  std::ofstream result;
  {
    std::string const pathname = "tests/out/assertions.txt";
    result = std::ofstream(pathname);
    assert_file<std::ofstream>(&result, pathname.c_str());
    test::set_ofstream(&result);
  }

  {
    using asc::Simulation, asc::Rule;
    std::vector<Assertion> assertions{};

    { // https://mathworld.wolfram.com/images/gifs/langant.gif
      uint_fast16_t const gridWidth = 7, gridHeight = 8;
      Simulation sim(
        gridWidth, gridHeight, 1,
        2, 2, AO_NORTH,
        std::array<Rule, 256>{{
          Rule(1, TD_RIGHT),
          Rule(0, TD_LEFT),
        }}
      );
      LOOP_N(50, sim.step_once());
      uint8_t const expectedGrid[] {
        1,1,1,1,1,1,1,
        1,1,0,0,1,1,1,
        1,0,1,1,0,1,1,
        1,0,1,1,1,0,1,
        1,1,0,1,1,0,1,
        1,0,1,1,0,1,1,
        1,1,0,0,1,1,1,
        1,1,1,1,1,1,1,
      };
      assertions.emplace_back("LR",
        arr2d::cmp(
          expectedGrid, sim.grid(), gridWidth, gridHeight
        ) &&
        sim.ant_col() == 1 &&
        sim.ant_row() == 3 &&
        sim.ant_orientation() == AO_NORTH
      );
    }

    run_suite("ant-simulator-core", assertions);
  }

  return 0;
}
