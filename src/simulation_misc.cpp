#include "simulation.hpp"

char const *simulation::orientation::to_string(
  orientation::value_type const orient)
{
  switch (orient) {
    case orientation::NORTH:
      return "north";
    case orientation::EAST:
      return "east";
    case orientation::SOUTH:
      return "south";
    case orientation::WEST:
      return "west";

    default:
    case orientation::OVERFLOW_CLOCKWISE:
    case orientation::OVERFLOW_COUNTER_CLOCKWISE:
      throw std::runtime_error("orientation::to_string failed - bad value");
  }
}

char const *simulation::turn_direction::to_string(
  turn_direction::value_type const turn_dir)
{
  switch (turn_dir) {
    case turn_direction::NIL:
      return "nil";
    case turn_direction::LEFT:
      return "L";
    case turn_direction::NO_CHANGE:
      return "N";
    case turn_direction::RIGHT:
      return "R";

    default:
      throw std::runtime_error("turn_direction::to_string failed - bad value");
  }
}

char const *simulation::step_result::to_string(
  step_result::value_type const step_res)
{
  switch (step_res) {
    case step_result::NIL:
      return "nil";
    case step_result::SUCCESS:
      return "success";
    case step_result::HIT_EDGE:
      return "hit_edge";
    default:
      throw std::runtime_error("step_result::to_string failed - bad value");
  }
}

b8 simulation::rule::operator!=(simulation::rule const &other) const noexcept
{
  return this->replacement_shade != other.replacement_shade || this->turn_dir != other.turn_dir;
}

namespace simulation
{
  // define ostream insertion operator for the rule struct, for ntest
  std::ostream &
  operator<<(std::ostream &os, simulation::rule const &rule)
  {
    os << '(' << static_cast<i32>(rule.replacement_shade)
       << ", " << simulation::turn_direction::to_string(rule.turn_dir) << ')';
    return os;
  }
}

b8 simulation::state::can_step_forward(u64 const generation_target) const noexcept
{
  return
    (last_step_res <= simulation::step_result::SUCCESS)
    && (generation < generation_target);
}
