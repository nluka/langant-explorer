#include <iostream>

#include "exit.hpp"
#include "simulation.hpp"
#include "term.hpp"
#include "util.hpp"

int main(int const argc, char const *const *const argv) {
  if (argc != 3 + 1 /* executable pathname */) {
    using namespace term::color;
    printf(
      fore::RED | back::BLACK,
      "usage: <sim_file> <iteration_target> <img_fmt>\n"
      "  <sim_file> = pathname to simulation file\n"
      "  <iteration_target> = uint64 in range [1, UINT64_MAX]\n"
      "  <img_fmt> = PGM image format, plain|raw (raw recommended)\n"
    );
    EXIT(ExitCode::WRONG_NUM_OF_ARGS);
  }

  auto [sim, errors] = [argv]() {
    std::string content = util::extract_txt_file_contents(argv[1]);
    return parse_simulation(content);
  }();

  if (!errors.empty()) {
    using namespace term::color;
    for (auto const &err : errors)
      printf(fore::RED | back::BLACK, err.c_str());
    EXIT(ExitCode::BAD_SIM_FILE);
  }

  uint_fast64_t const iteration_target = std::stoull(argv[2]);

  step_result::type step_res = step_result::NIL;
  #if 1
  do {
    step_res = step_forward(sim);

  } while (
    step_res == step_result::SUCCESS &&
    sim.generations < iteration_target
  );
  #else
  while (true) {
    step_res = step_forward(sim);
    if (
      step_res != StepResult::SUCCESS ||
      sim.generation == iteration_target
    ) [[unlikely]] {
      break;
    }
  }
  #endif

  EXIT(ExitCode::SUCCESS);
}
