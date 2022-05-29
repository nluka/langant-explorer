#ifndef ANT_SIMULATOR_CORE_HPP
#define ANT_SIMULATOR_CORE_HPP

#include <string>
#include <cinttypes>
#include <vector>
#include <array>
#include <fstream>

namespace asc { // stands for `ant simulator core`

void set_save_path(char const *);

//      AO stands for `ant orientation`
#define AO_OVERFLOW_COUNTER_CLOCKWISE static_cast<int_fast8_t>(-1)
#define AO_NORTH                      static_cast<int_fast8_t>( 0)
#define AO_EAST                       static_cast<int_fast8_t>( 1)
#define AO_SOUTH                      static_cast<int_fast8_t>( 2)
#define AO_WEST                       static_cast<int_fast8_t>( 3)
#define AO_OVERFLOW_CLOCKWISE         static_cast<int_fast8_t>( 4)

//      TD stands for `turn direction`
#define TD_LEFT   static_cast<int_fast8_t>(-1)
#define TD_NONE   static_cast<int_fast8_t>( 0)
#define TD_RIGHT  static_cast<int_fast8_t>( 1)

enum class StepResult {
  NIL = -1,
  SUCCESS,
  FAILED_AT_BOUNDARY
};

char const *step_result_to_string(StepResult);
char const *ant_orient_to_string(int_fast8_t);
char const *turn_dir_to_string(int_fast8_t);

struct Rule {
  bool m_isDefined; // false if rule is not in use
  uint8_t m_replacementShade;
  int_fast8_t m_turnDirection;

  Rule();
  Rule(uint8_t replacementShade, int_fast8_t turnDirection);
};

class Simulation {
protected:
  std::string m_name;
  // we use fast int types when possible because arithmetic on these values
  // needs to be as fast as possible as it may be repeated
  // millions to trillions of times
  uint_fast64_t m_iterationsCompleted = 0;
  StepResult m_mostRecentStepResult = StepResult::NIL;
  uint_fast16_t m_gridWidth, m_gridHeight;
  // 2D array but allocated as a single contiguous block of memory
  uint8_t *m_grid;
  uint_fast16_t m_antCol, m_antRow;
  int_fast8_t m_antOrientation;
  // hash table for shade rules, there are 256 shades so we can use their 8-bit
  // values as indices for their rules
  // i.e rule for shade N is m_rules[N] where (0 <= N <= 255)
  std::array<Rule, 256> m_rules;
  std::vector<uint_fast64_t> m_singularSnapshots;
  std::vector<uint_fast64_t> m_periodicSnapshots;
  int m_nextSingularSnapshotIdx;

  bool is_col_in_grid_bounds(int);
  bool is_row_in_grid_bounds(int);

public:
  Simulation();
  Simulation(
    std::string const &name,
    uint_fast64_t iterationsCompleted,
    uint_fast16_t gridWidth,
    uint_fast16_t gridHeight,
    uint8_t gridInitialShade,
    uint_fast16_t antStartingCol,
    uint_fast16_t antStartingRow,
    int_fast8_t antOrientation,
    // quicker to just memcpy so take as lvalue ref
    std::array<Rule, 256> const &rules,
    std::vector<uint_fast64_t> &&singularSnapshots,
    std::vector<uint_fast64_t> &&periodicSnapshots
  );

  std::string const &name() const;
  uint_fast64_t iterations_completed() const;
  StepResult last_step_result() const;
  uint_fast16_t grid_width() const;
  uint_fast16_t grid_height() const;
  uint_fast16_t ant_col() const;
  uint_fast16_t ant_row() const;
  int_fast8_t ant_orientation() const;
  uint8_t const *grid() const;
  bool is_finished() const;
  void save() const;
  void step_once();
};

} // namespace asc

#endif // ANT_SIMULATOR_CORE_HPP
