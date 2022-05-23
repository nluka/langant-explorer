#ifndef ANT_SIMULATOR_CORE_HPP
#define ANT_SIMULATOR_CORE_HPP

#include <string>
#include <cinttypes>
#include <vector>
#include <array>

namespace asc { // stands for `ant simulator core`

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

char const *step_result_to_string(StepResult res);

struct Rule {
  bool        m_isDefined; // false if rule is not in use
  uint8_t     m_replacementColor;
  int_fast8_t m_turnDirection;

  Rule();
  Rule(uint8_t replacementColor, int_fast8_t turnDirection);
};

class Simulation {
protected:
  // we use fast int types when possible because arithmetic on these values
  // needs to be as fast as possible as it may be repeated
  // millions to trillions of times
  uint_fast64_t           m_iterationsCompleted = 0;
  StepResult              m_mostRecentStepResult = StepResult::NIL;
  uint_fast16_t           m_gridWidth, m_gridHeight;
  // would like to use uint_fast8_t here, but this may increase the memory
  // requirement for this grid by several factors which is unacceptable.
  // would also like to use vector, but I need to control EXACTLY how much
  // memory to allocate upfront - vector does not give me this control.
  uint8_t *               m_grid;
  uint_fast16_t           m_antCol, m_antRow;
  int_fast8_t             m_antOrientation;
  // acts like a hash table, there are 256 colors so we can use the 8-bit
  // color values as indices for their rules
  // i.e rule for color value N is m_rules[N] where (0 <= N <= 255)
  std::array<Rule, 256>   m_rules;

  bool is_col_in_grid_bounds(int col);
  bool is_row_in_grid_bounds(int row);

public:
  Simulation();
  Simulation(
    uint_fast16_t               gridWidth,
    uint_fast16_t               gridHeight,
    uint8_t                     gridInitialColor,
    uint_fast16_t               antStartingCol,
    uint_fast16_t               antStartingRow,
    int_fast8_t                 antOrientation,
    std::array<Rule, 256> const &rules
  );
  bool is_finished() const;
  void step_once();
  StepResult last_step_result() const;
  uint8_t const *grid() const;
};

} // namespace asc

#endif // ANT_SIMULATOR_CORE_HPP
