#include <sstream>
#include <algorithm>
#include "ant-simulator-core.hpp"

using asc::Simulation, asc::Rule, asc::StepResult;

char const *asc::step_result_to_string(StepResult const res) {
  switch (res) {
    case StepResult::NIL: return "nil";
    case StepResult::SUCCESS: return "success";
    case StepResult::FAILED_AT_BOUNDARY: return "hit boundary";
    default: return nullptr;
  }
}

Rule::Rule()
: m_isDefined{false},
  m_replacementColor{},
  m_turnDirection{}
{}

Rule::Rule(
  uint8_t const replacementColor,
  int_fast8_t const turnDirection
) :
  m_replacementColor{replacementColor},
  m_turnDirection{turnDirection}
{}

bool Simulation::is_col_in_grid_bounds(int const col) {
  return col >= 0 && col < static_cast<int>(m_gridWidth - 1);
}
bool Simulation::is_row_in_grid_bounds(int const row) {
  return row >= 0 && row < static_cast<int>(m_gridHeight - 1);
}

Simulation::Simulation()
: m_gridWidth{0},
  m_gridHeight{0},
  m_grid{nullptr},
  m_antCol{0},
  m_antRow{0},
  m_antOrientation{},
  m_rules{}
{}

Simulation::Simulation(
  uint_fast16_t const          gridWidth,
  uint_fast16_t const          gridHeight,
  uint8_t const                gridInitialColor,
  uint_fast16_t const          antStartingCol,
  uint_fast16_t const          antStartingRow,
  int_fast8_t const            antOrientation,
  std::array<Rule, 256> const &rules
) :
  m_gridWidth{gridWidth},
  m_gridHeight{gridHeight},
  m_grid{nullptr},
  m_antCol{antStartingCol},
  m_antRow{antStartingRow},
  m_antOrientation{antOrientation},
  m_rules{rules}
{
  std::stringstream ss;

  // validate grid dimensions
  if (gridWidth < 1 || gridWidth > UINT16_MAX)
    ss << "gridWidth (" << gridWidth << ") not in range [1, " << UINT16_MAX << "]";
  else if (gridHeight < 1 || gridHeight > UINT16_MAX)
    ss << "gridHeight (" << gridHeight << ") not in range [1, " << UINT16_MAX << "]";
  // validate ant starting coords
  else if (!is_col_in_grid_bounds(antStartingCol))
    ss << "antStartingCol (" << antStartingCol << ") not on grid";
  else if (!is_row_in_grid_bounds(antStartingRow))
    ss << "antStartingRow (" << antStartingRow << ") not on grid";

  std::string const err = ss.str();
  if (!err.empty()) {
    throw err;
  }

  size_t const cellCount = gridWidth * gridHeight;
  m_grid = new uint8_t[cellCount];
  if (m_grid == nullptr) {
    ss << "not enough memory for grid";
    throw ss.str();
  }
  std::fill_n(m_grid, cellCount, gridInitialColor);
}

bool Simulation::is_finished() const {
  return m_mostRecentStepResult > StepResult::SUCCESS;
}

void Simulation::step_once() {
  size_t const currCellIndex = (m_antRow * m_gridWidth) + m_antCol;
  uint8_t const currCellColor = m_grid[currCellIndex];
  auto const &currCellRule = m_rules[currCellColor];

  { // turn
    m_antOrientation = m_antOrientation + currCellRule.m_turnDirection;
    if (m_antOrientation == AO_OVERFLOW_COUNTER_CLOCKWISE) {
      m_antOrientation = AO_WEST;
    } else if (m_antOrientation == AO_OVERFLOW_CLOCKWISE) {
      m_antOrientation = AO_NORTH;
    }
  }

  // update current cell color
  m_grid[currCellIndex] = currCellRule.m_replacementColor;

  { // try to move to next cell
    int nextCol;
    if (m_antOrientation == AO_EAST) {
      nextCol = static_cast<int>(m_antCol) + 1;
    } else if (m_antOrientation == AO_WEST) {
      nextCol = static_cast<int>(m_antCol) - 1;
    } else {
      nextCol = static_cast<int>(m_antCol);
    }

    int nextRow;
    if (m_antOrientation == AO_NORTH) {
      nextRow = static_cast<int>(m_antRow) - 1;
    } else if (m_antOrientation == AO_SOUTH) {
      nextRow = static_cast<int>(m_antRow) + 1;
    } else {
      nextRow = static_cast<int>(m_antRow);
    }

    if (
      !is_col_in_grid_bounds(nextCol) ||
      !is_row_in_grid_bounds(nextRow)
    ) {
      m_mostRecentStepResult = StepResult::FAILED_AT_BOUNDARY;
    } else {
      m_antCol = static_cast<uint16_t>(nextCol);
      m_antRow = static_cast<uint16_t>(nextRow);
      m_mostRecentStepResult = StepResult::SUCCESS;
    }
  }
}

StepResult Simulation::last_step_result() const {
  return m_mostRecentStepResult;
}

uint8_t const *Simulation::grid() const {
  return m_grid;
}
