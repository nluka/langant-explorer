#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ant.hpp"
#include "exit.hpp"
#include "term.hpp"
#include "util.hpp"

int main(int const argc, char const *const *const argv) {
  if (argc != 3 + 1 /* executable pathname */) {
    using namespace term::color;
    printf(
      fore::RED | back::BLACK,
      "usage: <name> <sim_file> <generation_target> [<img_fmt>]\n"
      "  <name> = name of simulation\n"
      "  <sim_file> = pathname to simulation JSON file\n"
      "  <generation_target> = uint64 in range [1, UINT64_MAX]\n"
      // "  <img_fmt> = PGM image format, plain|raw (raw is faster)\n"
    );
    EXIT(ExitCode::WRONG_NUM_OF_ARGS);
  }

  char const
    *const name = argv[1],
    *const sim_file_pathname = argv[2],
    *const generation_target_str = argv[3]//,
    // *const img_fmt_str = argv[4]
  ;

  auto const current_path = std::filesystem::current_path();

  try {
    auto parse_result = ant::simulation_parse(
      util::extract_txt_file_contents(sim_file_pathname),
      current_path
    );

    ant::simulation &sim = parse_result.first;
    std::vector<std::string> const &errors = parse_result.second;

    if (!errors.empty()) {
      using namespace term::color;
      for (auto const &err : errors)
        printf(fore::RED | back::BLACK, err.c_str());
      EXIT(ExitCode::BAD_SIM_FILE);
    }

    uint_fast64_t const generation_target = std::stoull(generation_target_str);

    std::thread sim_thread([&sim, name, generation_target, &current_path]() {
      ant::simulation_run(
        sim,
        name,
        generation_target,
        pgm8::format::RAW,
        current_path
      );
    });

    auto const update_ui = [name, generation_target, &sim]() {
      term::cursor::hide();

      // static uint_fast64_t prev_generations = 0;
      // static auto prev_timepoint = std::chrono::steady_clock::now();

      while (true) {
        auto const generations_thus_far = sim.generations;
        auto const percent_completion = (generations_thus_far / (double)generation_target) * 100.0;

        term::clear_curr_line();
        printf("name: %s\n", name);

        term::clear_curr_line();
        printf("generations: %llu / %llu (%.1lf%%)\n", generations_thus_far, generation_target, percent_completion);

        if (
          sim.last_step_res > ant::step_result::SUCCESS ||
          generations_thus_far >= generation_target
        ) {
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        term::cursor::move_up(2);
      }

      term::cursor::show();
    };

    std::thread ui_update_thread(update_ui);

    ui_update_thread.join();
    sim_thread.join();

    if (sim.last_step_res != ant::step_result::SUCCESS) {
      printf("simulation terminated early: %s\n", ant::step_result::to_string(sim.last_step_res));
      ant::simulation_save(sim, name, current_path, pgm8::format::RAW);
    }

    EXIT(ExitCode::SUCCESS);

  } catch (std::invalid_argument const &err) {
    term::color::printf(term::color::fore::RED, "fatal: %s", err.what());
    EXIT(ExitCode::BAD_ARG_VALUE);
  } catch (std::runtime_error const &err) {
    term::color::printf(term::color::fore::RED, "fatal: %s", err.what());
    EXIT(ExitCode::GENERIC_ERROR);
  } catch (...) {
    term::color::printf(term::color::fore::RED, "fatal: unknown error occurred");
    EXIT(ExitCode::UNKNOWN_ERROR);
  }
}
