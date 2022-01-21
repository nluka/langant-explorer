#ifndef ANT_SIMULATOR_LITE_CORE_SIMULATION_HPP
#define ANT_SIMULATOR_LITE_CORE_SIMULATION_HPP

#include <inttypes.h>
#include <stdbool.h>
#include "Ant.hpp"
#include "Grid.hpp"
#include "Ruleset.hpp"
#include "../constants.hpp"

enum class SimulationImageFileType {
  PgmAscii,
  PgmBinary
};

enum class SimulationIterationStatus {
  NotStarted = 0,
  InProgress,
  FinishedHitBoundary,
  FinishedReachedMax,
};

enum class SimulationImageStatus {
  Disabled = -2,
  FailedToOpenFile,
  NotStarted,
  InProgress,
  Finished
};

enum class SimulationErrorStatus {
  None = 0,
  FailedToAllocateMemoryForGridCells,
  FailedToOpenImageFile
};

struct Simulation {
  char                      name[256];
  SimulationImageFileType   imageFileType;
  SimulationIterationStatus iterationStatus = SimulationIterationStatus::NotStarted;
  SimulationImageStatus     imageStatus;
  SimulationErrorStatus     errorStatus = SimulationErrorStatus::None;
  uint64_t                  iterationsCompletedCount;
  uint64_t                  maxIterationsCount;
  Grid                      grid;
  Ant                       ant;
  Ruleset                   ruleset;
};

void   simulation_init(Simulation * sim);
void   simulation_run(Simulation * sim);
bool   simulation_is_finished(Simulation const * sim);
size_t simulation_print(Simulation const * sim);

#endif // ANT_SIMULATOR_LITE_CORE_SIMULATION_HPP
