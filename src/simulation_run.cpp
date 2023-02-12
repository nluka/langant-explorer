#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "lib/json.hpp"

#include "util.hpp"
#include "simulation.hpp"

namespace fs = std::filesystem;

namespace helpers
{
  // Finds the index of the smallest value in an array.
  template <typename Ty>
  usize idx_of_smallest(Ty const *const values, usize const num_values)
  {
    assert(num_values > 0);
    assert(values != nullptr);

    usize min_idx = 0;
    for (usize i = 1; i < num_values; ++i) {
      if (values[i] < values[min_idx]) {
        min_idx = i;
      }
    }

    return min_idx;
  }

  // Removes duplicate elements from a sorted vector.
  template <typename ElemTy>
  std::vector<ElemTy> remove_duplicates_sorted(std::vector<ElemTy> const &input)
  {
    if (input.size() < 2) {
      return input;
    }

    std::vector<ElemTy> output(input.size());

    // check the first element individually
    if (input[0] != input[1]) {
      output.push_back(input[0]);
    }

    for (usize i = 1; i < input.size() - 1; ++i) {
      if (input[i] != input[i - 1] && input[i] != input[i + 1]) {
        output.push_back(input[i]);
      }
    }

    // check the last item individually
    if (input[input.size() - 1] != input[input.size() - 2]) {
      output.push_back(input[input.size() - 1]);
    }

    return output;
  }
}

/*
  Runs a simulation until either the `generation_target` is reached
  or the ant tries to step off the grid.

  `save_points` is a list of points at which to emit a save.

  `save_interval` specifies an interval to emit saves, 0 indicates no interval.

  `save_dir` is where any saves (.json and image files) are emitted.

  If `save_final_state` is true, the final state of the simulation is saved,
  regardless of `save_points` and `save_interval`. If the final state happens
  to be saved because of a save_point or the `save_interval`, a duplicate save is not made.
*/
void
simulation::run(
  state &state,
  std::string const name,
  u64 const generation_target,
  std::vector<u64> save_points,
  u64 const save_interval,
  pgm8::format const img_fmt,
  std::filesystem::path const &save_dir,
  b8 const save_final_cfg)
{
  // sort save_points in descending order, so we can pop them off the back as we complete them
  std::sort(save_points.begin(), save_points.end(), std::greater<u64>());
  save_points = helpers::remove_duplicates_sorted(save_points);

  u64 last_saved_gen = UINT64_MAX;
  u64 const generation_limit = generation_target == 0 ? (UINT64_MAX - 1) : generation_target;

  for (;;) {
    // the most generations we can perform before we overflow state.generation
    u64 const max_dist = UINT64_MAX - state.generation;

    u64 const dist_to_next_save_interval =
      save_interval == 0 ? max_dist : save_interval - (state.generation % save_interval);

    u64 const dist_to_next_save_point =
      save_points.empty() ? max_dist : save_points.back() - state.generation;

    u64 const dist_to_gen_limit = generation_limit - state.generation;

    std::array<u64, 3> const distances{
      dist_to_next_save_interval,
      dist_to_next_save_point,
      dist_to_gen_limit,
    };

    enum stop_reason : usize
    {
      SAVE_INTERVAL = 0,
      SAVE_POINT,
      GENERATION_TARGET,
    };

    struct stop
    {
      u64 distance;
      stop_reason reason;
    };

    stop next_stop;
    usize const idx_of_smallest_dist = helpers::idx_of_smallest(distances.data(), distances.size());
    next_stop.distance = distances[idx_of_smallest_dist];
    next_stop.reason = static_cast<stop_reason>(idx_of_smallest_dist);

    if (next_stop.reason == stop_reason::SAVE_POINT) {
      save_points.pop_back();
    }

    for (usize i = 0; i < next_stop.distance; ++i) {
      state.last_step_res = simulation::attempt_step_forward(state);
      if (state.last_step_res == simulation::step_result::SUCCESS) [[likely]] {
        ++state.generation;
      } else [[unlikely]] {
        goto done;
      }
    }

    if (next_stop.reason != stop_reason::GENERATION_TARGET) {
      simulation::save_state(state, name.c_str(), save_dir, img_fmt);
      last_saved_gen = state.generation;
    } else {
      goto done;
    }
  }

done:

  b8 const final_state_already_saved = last_saved_gen == state.generation;
  if (save_final_cfg && !final_state_already_saved) {
    simulation::save_state(state, name.c_str(), save_dir, img_fmt);
  }
}

simulation::step_result::value_type simulation::attempt_step_forward(simulation::state &state)
{
  usize const curr_cell_idx = (state.ant_row * state.grid_width) + state.ant_col;
  u8 const curr_cell_shade = state.grid[curr_cell_idx];
  auto const &curr_cell_rule = state.rules[curr_cell_shade];

  // turn
  state.ant_orientation = state.ant_orientation + curr_cell_rule.turn_dir;
  if (state.ant_orientation == orientation::OVERFLOW_COUNTER_CLOCKWISE)
    state.ant_orientation = orientation::WEST;
  else if (state.ant_orientation == orientation::OVERFLOW_CLOCKWISE)
    state.ant_orientation = orientation::NORTH;

  // update current cell shade
  state.grid[curr_cell_idx] = curr_cell_rule.replacement_shade;

#if 1

  i32 next_col, next_row;
  if (state.ant_orientation == orientation::NORTH) {
    next_col = state.ant_col;
    next_row = state.ant_row - 1;
  } else if (state.ant_orientation == orientation::EAST) {
    next_col = state.ant_col + 1;
    next_row = state.ant_row;
  } else if (state.ant_orientation == orientation::SOUTH) {
    next_row = state.ant_row + 1;
    next_col = state.ant_col;
  } else { // west
    next_col = state.ant_col - 1;
    next_row = state.ant_row;
  }

#else

  i32 next_col;
  if (state.ant_orientation == orientation::EAST)
    next_col = state.ant_col + 1;
  else if (state.ant_orientation == orientation::WEST)
    next_col = state.ant_col - 1;
  else
    next_col = state.ant_col;

  int next_row;
  if (state.ant_orientation == orientation::NORTH)
    next_row = state.ant_row - 1;
  else if (state.ant_orientation == orientation::SOUTH)
    next_row = state.ant_row + 1;
  else
    next_row = state.ant_row;

#endif

  if (
    util::in_range_incl_excl(next_col, 0, state.grid_width) &&
    util::in_range_incl_excl(next_row, 0, state.grid_height)) [[likely]] {
    state.ant_col = next_col;
    state.ant_row = next_row;
    return step_result::SUCCESS;
  } else [[unlikely]] {
    return step_result::HIT_EDGE;
  }
}
