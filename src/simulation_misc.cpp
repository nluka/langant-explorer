#include "simulation.hpp"

char const *simulation::orientation::to_string(
  orientation::value_type const orient)
{
  switch (orient) {
    case orientation::NORTH: return "N";
    case orientation::EAST: return "E";
    case orientation::SOUTH: return "S";
    case orientation::WEST: return "W";

    default:
      throw std::runtime_error("orientation::to_string failed - bad orient");
  }
}

simulation::orientation::value_type simulation::orientation::from_string(char const *str)
{
  if (strcmp(str, "N") == 0) return orientation::NORTH;
  if (strcmp(str, "E") == 0) return orientation::EAST;
  if (strcmp(str, "S") == 0) return orientation::SOUTH;
  if (strcmp(str, "W") == 0) return orientation::WEST;
  throw std::runtime_error("orientation::from_string failed - bad str");
}

char const *simulation::turn_direction::to_string(
  turn_direction::value_type const turn_dir)
{
  switch (turn_dir) {
    case turn_direction::LEFT: return "L";
    case turn_direction::NO_CHANGE: return "N";
    case turn_direction::RIGHT: return "R";

    default:
      throw std::runtime_error("turn_direction::to_string failed - bad turn_dir");
  }
}

simulation::turn_direction::value_type simulation::turn_direction::from_char(char const ch)
{
  switch (ch) {
    case 'L': return turn_direction::LEFT;
    case 'N': return turn_direction::NO_CHANGE;
    case 'R': return turn_direction::RIGHT;

    default:
      throw std::runtime_error("turn_direction::from_char failed - bad ch");
  }
}

char const *simulation::step_result::to_string(
  step_result::value_type const step_res)
{
  switch (step_res) {
    case step_result::NIL: return "nil";
    case step_result::SUCCESS: return "success";
    case step_result::HIT_EDGE: return "hit_edge";

    default:
      throw std::runtime_error("step_result::to_string failed - bad step_res");
  }
}

b8 simulation::rule::operator!=(simulation::rule const &other) const noexcept
{
  return this->replacement_shade != other.replacement_shade || this->turn_dir != other.turn_dir;
}

namespace simulation
{
  // define ostream insertion operator for the rule struct, for ntest
  std::ostream &operator<<(std::ostream &os, simulation::rule const &rule)
  {
    os << '(' << static_cast<i32>(rule.replacement_shade)
       << ", " << static_cast<i32>(rule.turn_dir) << ')';
    return os;
  }
}

b8 simulation::state::can_step_forward(u64 const generation_target) const noexcept
{
  return
    (last_step_res <= simulation::step_result::SUCCESS)
    && (generation < generation_target);
}

u8 simulation::deduce_maxval_from_rules(simulation::rules_t const &rules)
{
  for (usize i = rules.size() - 1; i > 0; --i)
    if (rules[i].turn_dir != simulation::turn_direction::NIL)
      return i;
}
