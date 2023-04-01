// This module provides the data structures for options for the various programs,
// as well as functions for parsing command line arguments into said data structures
// and printing option descriptions as part of help messages.

#ifndef PROGRAM_OPTIONS_HPP
#define PROGRAM_OPTIONS_HPP

#include <string>

#include <boost/program_options.hpp>
#include "lib/pgm8.hpp"

#include "util.hpp"

namespace po
{
  using options_description = boost::program_options::options_description;

  options_description make_image_options_description();

  options_description make_states_options_description();

  options_description simulate_one_options_description();

  options_description simulate_many_options_description();

  struct make_image_options
  {
    std::string out_file_path;
    std::string content;
    i16 fill_value;
    u16 width;
    u16 height;
    pgm8::format format;
    u8 maxval;
  };

  void parse_make_image_options(
    int argc,
    char const *const *argv,
    make_image_options &,
    util::errors_t &);

  struct make_states_options
  {
    std::string name_mode;
    std::string grid_state;
    std::string turn_directions;
    std::string shade_order;
    std::string ant_orientations;
    std::string out_dir_path;
    std::string word_file_path;
    usize count;
    u16 min_num_rules;
    u16 max_num_rules;
    i32 grid_width;
    i32 grid_height;
    i32 ant_col;
    i32 ant_row;
    b8 create_dirs;
  };

  void parse_make_states_options(
    int argc,
    char const *const *argv,
    make_states_options &,
    util::errors_t &);

  struct simulation_options
  {
    std::string save_path;
    std::vector<u64> save_points;
    u64 generation_limit;
    u64 save_interval;
    pgm8::format image_format;
    b8 save_final_state;
    b8 create_logs;
    b8 save_image_only;
  };

  struct simulate_one_options
  {
    std::string name;
    std::string state_file_path;
    std::string log_file_path;
    simulation_options sim;
  };

  void parse_simulate_one_options(
    int argc,
    char const *const *argv,
    simulate_one_options &,
    util::errors_t &);

  struct simulate_many_options
  {
    std::string state_dir_path;
    std::string log_file_path;
    u32 num_threads;
    simulation_options sim;
  };

  void parse_simulate_many_options(
    int argc,
    char const *const *argv,
    simulate_many_options &,
    util::errors_t &);

} // namespace po

#endif // PROGRAM_OPTIONS_HPP
