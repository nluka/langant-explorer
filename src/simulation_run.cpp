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
#include "logger.hpp"

namespace fs = std::filesystem;

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

  for (u64 i = 1; i < input.size() - 1; ++i) {
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

// Finds the index of the smallest value in an array.
template <typename Ty>
u64 idx_of_smallest(Ty const *const values, u64 const num_values)
{
  assert(num_values > 0);
  assert(values != nullptr);

  u64 min_idx = 0;
  for (u64 i = 1; i < num_values; ++i) {
    if (values[i] < values[min_idx]) {
      min_idx = i;
    }
  }

  return min_idx;
}

simulation::run_result simulation::run(
  state &state,
  std::string const &name,
  u64 generation_limit,
  std::vector<u64> save_points,
  u64 const save_interval,
  pgm8::format const img_fmt,
  std::filesystem::path const &save_dir,
  b8 const save_final_cfg,
  b8 const create_logs,
  b8 const save_image_only)
{
  using logger::event_type;

  if (create_logs) {
    logger::log(event_type::SIM_START, name.c_str());
  }

  run_result result{};

  auto const begin_new_activity = [&](activity const activity) {
    state.current_activity = activity;
    state.activity_end = {};
    state.activity_start = util::current_time();
  };

  auto const end_curr_activity = [&]() {
    state.activity_end = util::current_time();
  };

  auto const compute_activity_duration_ns = [&]() {
    u64 const duration = util::nanos_between(state.activity_start, state.activity_end);
    return duration;
  };

  auto const do_save = [&] {
    try {
      simulation::save_state(state, name.c_str(), save_dir, img_fmt, save_image_only);
      ++result.num_save_points_successful;
      if (create_logs) {
        logger::log(logger::event_type::SAVE_PNT, "%s @ %zu", name.c_str(), state.generation);
      }
    } catch (std::runtime_error const &) {
      ++result.num_save_points_failed;
    } catch (...) {
      ++result.num_save_points_failed;
    }
  };

  // sort save_points in descending order, so we can pop them off the back as we complete them
  std::sort(save_points.begin(), save_points.end(), std::greater<u64>());
  save_points = remove_duplicates_sorted(save_points);

  u64 last_saved_gen = UINT64_MAX;
  if (generation_limit == 0) {
    generation_limit = (UINT64_MAX - 1);
  }

  state.maxval = deduce_maxval_from_rules(state.rules);

  if (state.generation >= generation_limit) {
    result.code = run_result::code::REACHED_GENERATION_LIMIT;
    goto done;
  }

  for (;;) {
    // the most generations we can perform before we overflow state.generation
    u64 const max_dist = UINT64_MAX - state.generation;

    u64 const dist_to_next_save_interval =
      save_interval == 0 ? max_dist : save_interval - (state.generation % save_interval);

    u64 const dist_to_next_save_point =
      save_points.empty() ? max_dist : save_points.back() - state.generation;

    u64 const dist_to_gen_limit = generation_limit - state.generation;

    std::array<u64, 3> const distances {
      dist_to_next_save_interval,
      dist_to_next_save_point,
      dist_to_gen_limit,
    };

    enum stop_reason : u64
    {
      SAVE_INTERVAL = 0,
      SAVE_POINT,
      GENERATION_LIMIT,
    };

    struct stop
    {
      u64 distance;
      stop_reason reason;
    };

    u64 const idx_of_smallest_dist = idx_of_smallest(distances.data(), distances.size());
    stop const next_stop {
      distances[idx_of_smallest_dist],
      static_cast<stop_reason>(idx_of_smallest_dist),
    };

    if (next_stop.reason == stop_reason::SAVE_POINT) {
      save_points.pop_back();
    }

    if (next_stop.distance > 0) {
      begin_new_activity(activity::ITERATING);

      for (u64 i = 0; i < next_stop.distance; ++i) {
        state.last_step_res = simulation::attempt_step_forward(state);
        if (state.last_step_res == simulation::step_result::SUCCESS) [[likely]] {
          ++state.generation;
        } else [[unlikely]] {
          break;
        }
      }

      end_curr_activity();
      u64 const iteration_duration_ns = compute_activity_duration_ns();
      state.nanos_spent_iterating += iteration_duration_ns;

      if (state.last_step_res == simulation::step_result::HIT_EDGE) {
        result.code = run_result::code::HIT_EDGE;
        goto done;
      }
    }

    if (next_stop.reason == stop_reason::SAVE_POINT || next_stop.reason == stop_reason::SAVE_INTERVAL) {
      begin_new_activity(activity::SAVING);
      do_save();
      end_curr_activity();
      last_saved_gen = state.generation;
      u64 const save_duration_ns = compute_activity_duration_ns();
      state.nanos_spent_saving += save_duration_ns;
    } else {
      result.code = run_result::code::REACHED_GENERATION_LIMIT;
      goto done;
    }
  }

done:

  b8 const final_state_already_saved = last_saved_gen == state.generation;
  if (save_final_cfg && !final_state_already_saved) {
    begin_new_activity(activity::SAVING);
    do_save();
    end_curr_activity();
    u64 const save_duration_ns = compute_activity_duration_ns();
    state.nanos_spent_saving += save_duration_ns;
  }
  begin_new_activity(activity::NIL);

  if (create_logs) {
    logger::log(
      event_type::SIM_END,
      "%s %s",
      name.c_str(),
      (result.code == simulation::run_result::REACHED_GENERATION_LIMIT
        ? util::make_str("reached generation limit of %zu", generation_limit).c_str()
        : util::make_str("hit edge @ %zu", state.generation).c_str())
    );
  }

  return result;
}

simulation::step_result::value_type simulation::attempt_step_forward(
  simulation::state &state)
{
  u64 const curr_cell_idx = (state.ant_row * state.grid_width) + state.ant_col;
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

#if 0

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

  i32 next_row;
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
