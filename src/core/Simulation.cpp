#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include "Simulation.hpp"
#include "event_log.hpp"
#include "PGM.hpp"
#include "../CommandVars.hpp"
#include "../util/util.hpp"
#include "../ui/ui.hpp"

void simulation_handle_error_status(Simulation * const sim) {
  switch (sim->errorStatus) {
    case SimulationErrorStatus::FailedToAllocateMemoryForGridCells:
      log_event(
        EventType::Error,
        "Simulation '%s' failed to start: unable to allocate memory for grid cells",
        sim->name
      );
      return;
    case SimulationErrorStatus::FailedToOpenImageFile:
      log_event(
        EventType::Error,
        "Simulation '%s' failed to open image file",
        sim->name
      );
      return;
    default:
      return;
  }
}

void simulation_iterate(Simulation * sim);
void simulation_emit_image(Simulation * sim);

void simulation_run(Simulation * const sim) {
  if (sim->errorStatus != SimulationErrorStatus::None) {
    return;
  }

  simulation_iterate(sim);

  if (g_comVars.outputImageFiles) {
    simulation_emit_image(sim);
    if (sim->errorStatus != SimulationErrorStatus::None) {
      simulation_handle_error_status(sim);
    }
  }

  grid_destroy(&sim->grid);
}

void simulation_init(Simulation * const sim) {
  if (!grid_init_cells(&sim->grid)) {
    sim->errorStatus = SimulationErrorStatus::FailedToAllocateMemoryForGridCells;
    simulation_handle_error_status(sim);
  }
  if (!g_comVars.outputImageFiles) {
    sim->imageStatus = SimulationImageStatus::Disabled;
  }
}

void simulation_iterate(Simulation * const sim) {
  sim->iterationStatus = SimulationIterationStatus::InProgress;

  log_event(EventType::Info, "Started simulation '%s'", sim->name);

  time_t const startTime = time(NULL);
  AntStepResult stepResult = AntStepResult::Ok;
  if (sim->maxIterationsCount == 0) {
    while (1) {
      stepResult = ant_take_step(&sim->ant, &sim->grid, &sim->ruleset);
      if (stepResult != AntStepResult::Ok) {
        break;
      }
      ++sim->iterationsCompletedCount;
    }
  } else {
    for (
      sim->iterationsCompletedCount = 0;
      sim->iterationsCompletedCount < sim->maxIterationsCount
        && stepResult == AntStepResult::Ok;
      ++sim->iterationsCompletedCount
    ) {
      stepResult = ant_take_step(&sim->ant, &sim->grid, &sim->ruleset);
    }
  }
  time_t const endTime = time(NULL);

  Timespan const iterationTimespan = timespan_calculate(endTime - startTime);
  std::string iterationTimespanStr = timespan_convert_to_string(&iterationTimespan);
  std::stringstream resultDescSs;

  switch (stepResult) {
    case AntStepResult::HitBoundary:
      sim->iterationStatus = SimulationIterationStatus::FinishedHitBoundary;
      resultDescSs << "ant hit a boundary after " << sim->iterationsCompletedCount << " iterations";
      break;
    case AntStepResult::Ok:
      sim->iterationStatus = SimulationIterationStatus::FinishedReachedMax;
      resultDescSs << "reached max iterations of " << sim->maxIterationsCount;
      break;
  }

  log_event(
    EventType::Success, "Finished simulation '%s': %s (took %s)",
    sim->name,
    resultDescSs.str().c_str(),
    iterationTimespanStr.c_str()
  );
}

void simulation_emit_image(Simulation * const sim) {
  std::string imagePathname;
  imagePathname.reserve(
    g_comVars.imageFileOutputPath.size() +
    strlen(sim->name) + 4
  );

  { // compute file pathname
    imagePathname += g_comVars.imageFileOutputPath;
    imagePathname += sim->name;
    imagePathname += ".pgm";
  }

  std::ofstream image;

  { // open file for writing
    auto const mode = sim->imageFileType == SimulationImageFileType::PgmAscii
      ? std::ios_base::out
      : std::ios_base::out | std::ios_base::binary;

    image.open(imagePathname, mode);
    assert_file(image, imagePathname.c_str(), false);
    if (!image.is_open()) {
      sim->imageStatus = SimulationImageStatus::FailedToOpenFile;
      return;
    }
  }

  { // write file
    sim->imageStatus = SimulationImageStatus::InProgress;

    void (* file_writer)(
      std::ofstream & file,
      uint16_t colCount,
      uint16_t rowCount,
      color_t maxPixelValue,
      color_t const * pixels
    ) = NULL;

    switch (sim->imageFileType) {
      case SimulationImageFileType::PgmAscii:
        file_writer = pgm_write_file_ascii;
        break;
      case SimulationImageFileType::PgmBinary:
        file_writer = pgm_write_file_binary;
        break;
    }

    {
      time_t const startTime = time(NULL);
      log_event(
        EventType::Info,
        "Started writing file '%s'",
        imagePathname.c_str()
      );

      file_writer(
        image,
        sim->grid.width,
        sim->grid.height,
        ruleset_find_largest_color(&sim->ruleset),
        sim->grid.cells
      );
      image.close();

      time_t const endTime = time(NULL);
      Timespan const writeTimespan = timespan_calculate(endTime - startTime);
      std::string writeTimespanStr = timespan_convert_to_string(&writeTimespan);
      log_event(
        EventType::Success,
        "Finished writing file '%s' (took %s)",
        imagePathname.c_str(),
        writeTimespanStr.c_str()
      );
    }

    sim->imageStatus = SimulationImageStatus::Finished;
  }
}

bool simulation_is_finished(Simulation const * const sim) {
  return sim->errorStatus != SimulationErrorStatus::None || (
    sim->iterationStatus >= SimulationIterationStatus::FinishedHitBoundary &&
    (
      sim->imageStatus < static_cast<SimulationImageStatus>(0) ||
      sim->imageStatus == SimulationImageStatus::Finished
    )
  );
}

size_t simulation_print(Simulation const * const sim) {
  size_t linesPrintedCount = 0;

  { // name
    stdout_clear_current_line();
    printfc(TextColor::Cyan, "%s\n", sim->name);
    ++linesPrintedCount;
  }

  if (sim->errorStatus != SimulationErrorStatus::None) {
    stdout_clear_current_line();
    switch (sim->errorStatus) {
      case SimulationErrorStatus::FailedToAllocateMemoryForGridCells:
        printfc(TextColor::Red, "failed to allocate memory for grid cells\n");
        break;
    }
    print_horizontal_rule();
    linesPrintedCount += 2;
    return linesPrintedCount;
  }

  { // iteration status
    stdout_clear_current_line();
    printf("iterations : ");
    switch (sim->iterationStatus) {
      case SimulationIterationStatus::NotStarted:
        printfc(TextColor::Gray, "not started");
        break;
      case SimulationIterationStatus::InProgress: {
        if (sim->maxIterationsCount == 0) {
          printf("%" PRIu64, sim->iterationsCompletedCount);
        } else {
          int const completionPercentage = static_cast<int>((
            (double)sim->iterationsCompletedCount /
            sim->maxIterationsCount
          ) * 100);
          printf("%d%%",  completionPercentage);
        }
        break;
      }
      case SimulationIterationStatus::FinishedHitBoundary:
        printfc(TextColor::Green, "finished (hit boundary)");
        break;
      case SimulationIterationStatus::FinishedReachedMax:
        printfc(TextColor::Green, "finished (reached max)");
        break;
    }
    printf("\n");
    ++linesPrintedCount;
  }

  { // image status
    stdout_clear_current_line();
    printf("image      : ");
    switch (sim->imageStatus) {
      case SimulationImageStatus::Disabled:
        printfc(TextColor::Gray, "disabled");
        break;
      case SimulationImageStatus::FailedToOpenFile:
        printfc(TextColor::Red, "failed");
        break;
      case SimulationImageStatus::NotStarted:
        printfc(TextColor::Gray, "not started");
        break;
      case SimulationImageStatus::InProgress:
        printfc(TextColor::Yellow, "in progress");
        break;
      case SimulationImageStatus::Finished:
        printfc(TextColor::Green, "finished");
        break;
    }
    printf("\n");
    ++linesPrintedCount;
  }

  print_horizontal_rule();
  ++linesPrintedCount;

  return linesPrintedCount;
}
