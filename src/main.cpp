#include <filesystem>
#include <iostream>

#include "ant.hpp"
#include "exit.hpp"
#include "term.hpp"
#include "util.hpp"

int main(int const argc, char const *const *const argv) {
  if (argc != 3 + 1 /* executable pathname */) {
    using namespace term::color;
    printf(
      fore::RED | back::BLACK,
      "usage: <name> <sim_file> <generation_target> <img_fmt>\n"
      "  <name> = name of simulation\n"
      "  <sim_file> = pathname to simulation file\n"
      "  <generation_target> = uint64 in range [1, UINT64_MAX]\n"
      "  <img_fmt> = PGM image format, plain|raw (raw recommended)\n"
    );
    EXIT(ExitCode::WRONG_NUM_OF_ARGS);
  }

  auto [sim, errors] = [argv]() {
    std::string const json_str = util::extract_txt_file_contents(argv[1]);
    return ant::simulation_parse(json_str, std::filesystem::current_path());
  }();

  if (!errors.empty()) {
    using namespace term::color;
    for (auto const &err : errors)
      printf(fore::RED | back::BLACK, err.c_str());
    EXIT(ExitCode::BAD_SIM_FILE);
  }

  uint_fast64_t const generation_target = std::stoull(argv[3]);
  ant::simulation_run(sim, argv[1], generation_target, pgm8::format::RAW, std::filesystem::current_path());

  EXIT(ExitCode::SUCCESS);
}
