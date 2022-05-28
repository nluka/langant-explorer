#include <sstream>
#include <algorithm>
#include "../cpp-lib/includes/pgm8.hpp"
#include "ant-simulator-core.hpp"

using asc::Simulation, asc::Rule, asc::StepResult;

char const *asc::step_result_to_string(StepResult const res) {
  switch (res) {
    case StepResult::NIL:                 return "nil";
    case StepResult::SUCCESS:             return "success";
    case StepResult::FAILED_AT_BOUNDARY:  return "hit boundary";
    default: throw "bad StepResult";
  }
}
char const *asc::ant_orient_to_string(int_fast8_t const orientation) {
  switch (orientation) {
    case AO_NORTH:  return "N";
    case AO_EAST:   return "E";
    case AO_SOUTH:  return "S";
    case AO_WEST:   return "W";
    default: throw "bad orientation";
  }
}
char const *asc::turn_dir_to_string(int_fast8_t turnDir) {
  switch (turnDir) {
    case TD_LEFT:  return "L";
    case TD_NONE:  return "N";
    case TD_RIGHT: return "R";
    default: throw "bad turn direction";
  }
}

Rule::Rule()
: m_isDefined{false},
  m_replacementShade{},
  m_turnDirection{}
{}

Rule::Rule(
  uint8_t const replacementShade,
  int_fast8_t const turnDirection
) :
  m_isDefined{true},
  m_replacementShade{replacementShade},
  m_turnDirection{turnDirection}
{}

bool Simulation::is_col_in_grid_bounds(int const col) {
  return col >= 0 && col < static_cast<int>(m_gridWidth - 1);
}
bool Simulation::is_row_in_grid_bounds(int const row) {
  return row >= 0 && row < static_cast<int>(m_gridHeight - 1);
}

Simulation::Simulation()
: m_name{},
  m_gridWidth{0},
  m_gridHeight{0},
  m_grid{nullptr},
  m_antCol{0},
  m_antRow{0},
  m_antOrientation{},
  m_rules{}
{}

Simulation::Simulation(
  std::string const &name,
  uint_fast16_t const gridWidth,
  uint_fast16_t const gridHeight,
  uint8_t const gridInitialShade,
  uint_fast16_t const antStartingCol,
  uint_fast16_t const antStartingRow,
  int_fast8_t const antOrientation,
  std::array<Rule, 256> const &rules
) :
  m_name{name},
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
  std::fill_n(m_grid, cellCount, gridInitialShade);
}

StepResult Simulation::last_step_result() const {
  return m_mostRecentStepResult;
}
uint_fast16_t Simulation::grid_width() const {
  return m_gridWidth;
}
uint_fast16_t Simulation::grid_height() const {
  return m_gridHeight;
}
uint_fast16_t Simulation::ant_col() const {
  return m_antCol;
}
uint_fast16_t Simulation::ant_row() const {
  return m_antRow;
}
int_fast8_t Simulation::ant_orientation() const {
  return m_antOrientation;
}
uint8_t const *Simulation::grid() const {
  return m_grid;
}
bool Simulation::is_finished() const {
  return m_mostRecentStepResult > StepResult::SUCCESS;
}

void Simulation::save(
  std::ofstream &fsim,
  std::ofstream &fpgm,
  std::string const &fpgmPathname
) const {
  bool const isGridHomogenous =
    arr2d::is_homogenous(m_grid, m_gridWidth, m_gridHeight);

  if (!isGridHomogenous) { // write PGM file
    // TODO: cache value instead of recomputing on each save
    uint8_t const maxval = ([this](){
      for (uint8_t shade = 255; shade >= 1; --shade) {
        auto const &rule = m_rules[shade];
        if (rule.m_isDefined) {
          return shade;
        }
      }
      return static_cast<uint8_t>(0);
    })();

    pgm8::write_ascii(
      &fpgm,
      static_cast<uint16_t>(m_gridWidth),
      static_cast<uint16_t>(m_gridHeight),
      maxval,
      m_grid
    );
  }

  { // write sim file
    char const *const delim = ";\n";

    fsim
      << "name = " << m_name << delim
      << "iteration = " << m_iterationsCompleted << delim
      << "lastStepResult = " <<
        asc::step_result_to_string(m_mostRecentStepResult) << delim
      << "gridWidth = " << m_gridWidth << delim
      << "gridHeight = " << m_gridHeight
    << delim;

    fsim << "gridState = ";
      if (isGridHomogenous) {
        fsim << std::to_string(m_grid[0]);
      } else {
        fsim << fpgmPathname;
      }
    fsim << delim;

    fsim
      << "antCol = " << m_antCol << delim
      << "antRow = " << m_antRow << delim
      << "antOrientation = "
        << ant_orient_to_string(m_antOrientation) << delim;

    fsim << "rules = [\n";
      for (size_t shade = 0; shade < m_rules.size(); ++shade) {
        auto const &rule = m_rules[shade];
        if (!rule.m_isDefined) {
          continue;
        }
        fsim << "  {"
          << shade << ','
          << std::to_string(rule.m_replacementShade) << ','
          << asc::turn_dir_to_string(rule.m_turnDirection)
        << "},\n";
      }
    fsim << ']' << delim;
  }
}

void Simulation::step_once() {
  size_t const currCellIdx = (m_antRow * m_gridWidth) + m_antCol;
  uint8_t const currCellShade = m_grid[currCellIdx];
  auto const &currCellRule = m_rules[currCellShade];

  { // turn
    m_antOrientation = m_antOrientation + currCellRule.m_turnDirection;
    if (m_antOrientation == AO_OVERFLOW_COUNTER_CLOCKWISE) {
      m_antOrientation = AO_WEST;
    } else if (m_antOrientation == AO_OVERFLOW_CLOCKWISE) {
      m_antOrientation = AO_NORTH;
    }
  }

  // update current cell shade
  m_grid[currCellIdx] = currCellRule.m_replacementShade;

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
