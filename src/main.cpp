#include <array>
#include <iostream>
#include "ant-simulator-core.hpp"
#include "cpp-lib/includes/pgm8.hpp"

using asc::Simulation, asc::Rule;

int main() {
  uint_fast16_t const gridWidth = 100, gridHeight = 100;

  Simulation sim(
    gridWidth, gridHeight, 1,
    50, 50, AO_WEST,
    std::array<Rule, 256>{
      { Rule(1, TD_LEFT), Rule(0, TD_RIGHT) }
    }
  );

  for (size_t i = 1; i <= 11000; ++i) {
    sim.step_once();
  }

  {
    char const *const fpathname = "out/LR.pgm";
    std::ofstream file(fpathname);
    if (!file.is_open() || !file.good()) {
      std::cerr << "ERROR: failed to open `" << fpathname << "`\n";
      return 1;
    }
    try {
      pgm8::write_ascii(&file, gridWidth, gridHeight, 1, sim.grid());
    } catch (char const *const err) {
      std::cerr << "ERROR: failed to write `" << fpathname << "`: " << err << '\n';
      return 2;
    }
  }

  return 0;
}
