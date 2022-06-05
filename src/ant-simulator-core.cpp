#include <sstream>
#include <algorithm>
#include <filesystem>
#include "../cpp-lib/includes/pgm8.hpp"
#include "ant-simulator-core.hpp"

namespace fs = std::filesystem;
using asc::Simulation, asc::Rule, asc::StepResult;

static fs::path s_savePath{};
void asc::set_save_path(char const *const p) {
  s_savePath = p;
}

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

void Simulation::validate_grid_width(
  std::vector<std::string> &errors,
  uint_fast16_t const gridWidth
) {
  if (gridWidth < 1 || gridWidth > UINT16_MAX) {
    std::stringstream ss{};
    ss << "gridWidth (" << gridWidth << ") not in range [1, " << UINT16_MAX << ']';
    errors.emplace_back(ss.str());
  }
}
void Simulation::validate_grid_height(
  std::vector<std::string> &errors,
  uint_fast16_t const gridHeight
) {
  if (gridHeight < 1 || gridHeight > UINT16_MAX) {
    std::stringstream ss{};
    ss << "gridHeight (" << gridHeight << ") not in range [1, " << UINT16_MAX << ']';
    errors.emplace_back(ss.str());
  }
}
void Simulation::validate_ant_col(
  std::vector<std::string> &errors,
  uint_fast16_t const antCol,
  uint_fast16_t const gridWidth
) {
  if (!is_coord_in_grid_dimension(antCol, gridWidth)) {
    std::stringstream ss{};
    ss << "antCol (" << antCol << ") not within gridWidth (" << gridWidth << ')';
    errors.emplace_back(ss.str());
  }
}
void Simulation::validate_ant_row(
  std::vector<std::string> &errors,
  uint_fast16_t const antRow,
  uint_fast16_t const gridHeight
) {
  if (!is_coord_in_grid_dimension(antRow, gridHeight)) {
    std::stringstream ss{};
    ss << "antRow (" << antRow << ") not within gridHeight (" << gridHeight << ')';
    errors.emplace_back(ss.str());
  }
}
void Simulation::validate_rules(
  std::vector<std::string> &errors,
  std::array<Rule, 256> const &rules
) {
  std::array<uint16_t, 256> shadeOccurences{};
  size_t definedRules = 0;

  for (size_t i = 0; i < rules.size(); ++i) {
    auto const &r = rules[i];
    if (r.m_isDefined) {
      ++definedRules;
      ++shadeOccurences[i];
      ++shadeOccurences[r.m_replacementShade];
    }
  }

  if (definedRules < 2) {
    errors.emplace_back("fewer than 2 rules defined");
    return;
  }

  size_t nonZeroOccurences = 0, nonTwoOccurences = 0;
  for (auto const occurencesOfShade : shadeOccurences) {
    if (occurencesOfShade != 0) {
      ++nonZeroOccurences;
      if (occurencesOfShade != 2) {
        ++nonTwoOccurences;
      }
    }
  }

  if (nonZeroOccurences < 2 || nonTwoOccurences > 0) {
    errors.emplace_back("rules don't form a closed chain");
  }
}
bool Simulation::is_coord_in_grid_dimension(
  int const coord,
  uint_fast16_t const gridDimension
) {
  return coord >= 0 && coord < static_cast<int>(gridDimension);
}

Simulation::Simulation()
: m_name{},
  m_gridWidth{0},
  m_gridHeight{0},
  m_grid{nullptr},
  m_antCol{0},
  m_antRow{0},
  m_antOrientation{},
  m_rules{},
  m_nextSingularSnapshotIdx{-1}
{}

Simulation::Simulation(
  std::string const &name,
  uint_fast64_t iterationsCompleted,
  uint_fast16_t const gridWidth,
  uint_fast16_t const gridHeight,
  uint8_t const gridInitialShade,
  uint_fast16_t const antStartingCol,
  uint_fast16_t const antStartingRow,
  int_fast8_t const antOrientation,
  std::array<Rule, 256> const &rules,
  std::vector<uint_fast64_t> &&singularSnapshots,
  std::vector<uint_fast64_t> &&periodicSnapshots
) :
  m_name{name},
  m_iterationsCompleted{iterationsCompleted},
  m_gridWidth{gridWidth},
  m_gridHeight{gridHeight},
  m_grid{nullptr},
  m_antCol{antStartingCol},
  m_antRow{antStartingRow},
  m_antOrientation{antOrientation},
  m_rules{rules},
  m_singularSnapshots{singularSnapshots},
  m_periodicSnapshots{periodicSnapshots},
  m_nextSingularSnapshotIdx{-1}
{
  size_t const cellCount =
    static_cast<size_t>(gridWidth) * static_cast<size_t>(gridHeight);
  m_grid = new uint8_t[cellCount];
  if (m_grid == nullptr) {
    throw "not enough memory for grid";
  }
  std::fill_n(m_grid, cellCount, gridInitialShade);

  std::sort(m_singularSnapshots.begin(), m_singularSnapshots.end());
  std::sort(m_periodicSnapshots.begin(), m_periodicSnapshots.end());

  if (m_singularSnapshots.empty()) {
    m_nextSingularSnapshotIdx = -1;
  } else {
    if (m_iterationsCompleted < m_singularSnapshots.front()) {
      m_nextSingularSnapshotIdx = 0;
    } else if (m_iterationsCompleted >= m_singularSnapshots.back()) {
      m_nextSingularSnapshotIdx = -1;
    } else {
      for (size_t i = 0; i < m_singularSnapshots.size() - 2; ++i) {
        size_t const rightSnapIdx = i + 1;
        uint_fast64_t const
          leftSnap = m_singularSnapshots[i],
          rightSnap = m_singularSnapshots[rightSnapIdx];
        if (
          m_iterationsCompleted > leftSnap &&
          m_iterationsCompleted <= rightSnap
        ) {
          m_nextSingularSnapshotIdx = static_cast<int>(rightSnapIdx);
        }
      }
    }
  }
}

std::string const &Simulation::name() const {
  return m_name;
}
uint_fast64_t Simulation::iterations_completed() const {
  return m_iterationsCompleted;
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

void Simulation::save() const {
  std::string const fnameBase = m_name + '(' + std::to_string(m_iterationsCompleted) + ')';
  fs::path const fpgmPathname = s_savePath / (fnameBase + ".pgm");
  std::ofstream fsim(s_savePath / (fnameBase + ".sim"));

  bool const isGridHomogenous =
    arr2d::is_homogenous(m_grid, m_gridWidth, m_gridHeight);

  if (!isGridHomogenous) { // write PGM file
    // TODO: cache value instead of recomputing on each save
    uint8_t const maxval = ([this]() {
      for (uint8_t shade = 255; shade >= 1; --shade) {
        auto const &rule = m_rules[shade];
        if (rule.m_isDefined) {
          return shade;
        }
      }
      return static_cast<uint8_t>(0);
    })();

    std::ofstream fpgm(fpgmPathname);

    pgm8::write_ascii(
      &fpgm,
      static_cast<uint16_t>(m_gridWidth),
      static_cast<uint16_t>(m_gridHeight),
      maxval,
      m_grid
    );
  }

  { // write sim file
    // note that any properties prefixed with `lite`
    // are specific to `ant-simulator-lite`

    char const *const delim = ";\n", *const indent = "  ";

    fsim
      << "name = " << m_name << delim
      << "iterationsCompleted = " << m_iterationsCompleted << delim
      << "lastStepResult = " << asc::step_result_to_string(m_mostRecentStepResult) << delim
      << "gridWidth = " << m_gridWidth << delim
      << "gridHeight = " << m_gridHeight << delim
      << "gridState = ";
    if (isGridHomogenous) {
      // using << results in double quotes being added, so gotta do this instead
      std::string const val = std::to_string(m_grid[0]).c_str();
      fsim.write(val.c_str(), val.size());
    } else {
      fsim << fpgmPathname;
    }
    fsim << delim
      << "antCol = " << m_antCol << delim
      << "antRow = " << m_antRow << delim
      << "antOrientation = " << ant_orient_to_string(m_antOrientation) << delim;

    fsim << "rules = [\n";
    for (size_t shade = 0; shade < m_rules.size(); ++shade) {
      auto const &rule = m_rules[shade];
      if (rule.m_isDefined) {
        fsim << indent << '{'
        << shade << ','
        << std::to_string(rule.m_replacementShade) << ','
        << asc::turn_dir_to_string(rule.m_turnDirection)
        << "},\n";
      }
    }
    fsim << ']' << delim;

    fsim << "lite.singularSnapshots = [";
    if (m_singularSnapshots.size() > 0) {
      fsim << '\n';
    }
    for (auto const snap : m_singularSnapshots) {
      fsim << indent << snap << ",\n";
    }
    fsim << ']' << delim;

    fsim << "lite.periodicSnapshots = [";
    if (m_singularSnapshots.size() > 0) {
      fsim << '\n';
    }
    for (auto const snap : m_periodicSnapshots) {
      fsim << indent << snap << ",\n";
    }
    fsim << ']' << delim;
  }
}

void Simulation::step_once() {
  size_t const currCellIdx = (m_antRow * m_gridWidth) + m_antCol;
  uint8_t const currCellShade = m_grid[currCellIdx];
  auto const &currCellRule = m_rules[currCellShade];

  // turn
  m_antOrientation = m_antOrientation + currCellRule.m_turnDirection;
  if (m_antOrientation == AO_OVERFLOW_COUNTER_CLOCKWISE) {
    m_antOrientation = AO_WEST;
  } else if (m_antOrientation == AO_OVERFLOW_CLOCKWISE) {
    m_antOrientation = AO_NORTH;
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
      !is_coord_in_grid_dimension(nextCol, m_gridWidth) ||
      !is_coord_in_grid_dimension(nextRow, m_gridHeight)
    ) {
      m_mostRecentStepResult = StepResult::FAILED_AT_BOUNDARY;
    } else {
      m_antCol = static_cast<uint16_t>(nextCol);
      m_antRow = static_cast<uint16_t>(nextRow);
      m_mostRecentStepResult = StepResult::SUCCESS;
    }
  }

  ++m_iterationsCompleted;

  // take snapshot if applicable
  if (
    // time to take singular snapshot?
    m_nextSingularSnapshotIdx != -1 &&
    (m_singularSnapshots[m_nextSingularSnapshotIdx] ==
    m_iterationsCompleted)
  ) {
    save();
    if (m_nextSingularSnapshotIdx == m_singularSnapshots.size() - 1) {
      // that was the final singular snapshot
      m_nextSingularSnapshotIdx = -1;
    } else {
      // there are more singular snapshots later
      ++m_nextSingularSnapshotIdx;
    }
  } else if (
    // time to take periodic snapshot?
    !m_periodicSnapshots.empty() &&
    ([this]() {
      for (uint_fast64_t const period : m_periodicSnapshots) {
        if (m_iterationsCompleted % period == 0) {
          return true;
        }
      }
      return false;
    })()
  ) {
    save();
  }
}
