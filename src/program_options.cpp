#include <regex>

#include "lib/pgm8.hpp"
#include "lib/json.hpp"

#include "program_options.hpp"
#include "util.hpp"

using util::make_str;
using util::print_err;
using util::errors_t;
using po::options_description;
using json_t = nlohmann::json;

namespace fs = std::filesystem;

struct option
{
  char const *full_name;
  char const shorthand;

  std::string to_string() const
  {
    return make_str("-%c [ --%s ]", shorthand, full_name);
  }
};

namespace make_image
{
  option out_file_path() { return { "out_file_path", 'o' }; }
  option format()        { return { "format",        'f' }; }
  option content()       { return { "content",       'c' }; }
  option width()         { return { "width",         'w' }; }
  option height()        { return { "height",        'h' }; }
  option maxval()        { return { "maxval",        'm' }; }

  char const *regex_content() { return "^(noise)|(fill=[0-9]{1,3})$"; };
}

namespace make_states
{
  option name_mode()        { return { "name_mode",        'n' }; }
  option word_file_path()   { return { "word_file_path",   'W' }; }
  option grid_state()       { return { "grid_state",       'g' }; }
  option turn_directions()  { return { "turn_directions",  't' }; }
  option shade_order()      { return { "shade_order",      's' }; }
  option ant_orientations() { return { "ant_orientations", 'O' }; }
  option out_dir_path()     { return { "out_dir_path",     'o' }; }
  option create_dirs()      { return { "create_dirs",      'c' }; }
  option count()            { return { "count",            'N' }; }
  option min_num_rules()    { return { "min_num_rules",    'm' }; }
  option max_num_rules()    { return { "max_num_rules",    'M' }; }
  option grid_width()       { return { "grid_width",       'w' }; }
  option grid_height()      { return { "grid_height",      'h' }; }
  option ant_col()          { return { "ant_col",          'x' }; }
  option ant_row()          { return { "ant_row",          'y' }; }

  char const *name_mode_regex()        { return "^(turndirecs)|(randwords,[1-9])|(alpha,[1-9])$"; }
  char const *turn_directions_regex()  { return "^[lLnNrR]+$"; }
  char const *ant_orientations_regex() { return "^[nNeEsSwW]+$"; }

  char const *shade_order_default()      { return "asc";  }
  char const *turn_directions_default()  { return "LR";   }
  char const *ant_orientations_default() { return "NESW"; }
  u16         min_num_rules_default()    { return 2;      }
  u16         max_num_rules_default()    { return 256;    }
  i16         fill_value_default()       { return 0;      }
  std::string fill_string_default()      { return make_str("fill=%d", fill_value_default()); }
}

namespace simulation
{
  option generation_limit() { return { "generation_limit", 'g' }; }
  option image_format()     { return { "image_format",     'f' }; }
  option create_logs()      { return { "create_logs",      'l' }; }
  option save_path()        { return { "save_path",        'o' }; }
  option save_image_only()  { return { "save_image_only",  'y' }; }
  option save_final_state() { return { "save_final_state", 's' }; }
  option save_points()      { return { "save_points",      'p' }; }
  option save_interval()    { return { "save_interval",    'v' }; }
}

namespace simulate_one
{
  option name()            { return { "name",            'N' }; }
  option state_file_path() { return { "state_file_path", 'S' }; }
  option log_file_path()   { return { "log_file_path",   'L' }; }
}

namespace simulate_many
{
  option num_threads()    { return { "num_threads",    'T' }; }
  option state_dir_path() { return { "state_dir_path", 'S' }; }
  option log_file_path()  { return { "log_file_path",  'L' }; }
}

template <typename Ty>
std::optional<Ty> get_required_option(
  option const &opt,
  boost::program_options::variables_map const &vm,
  errors_t &errors)
{
  if (vm.count(opt.full_name) == 0) {
    errors.emplace_back(make_str("%s required", opt.to_string().c_str()));
    return std::nullopt;
  }

  try {
    return vm.at(opt.full_name).as<Ty>();
  } catch (std::exception const &except) {
    errors.emplace_back(make_str("%s %s", opt.to_string().c_str(), except.what()));
    return std::nullopt;
  }
}

template <typename Ty>
std::optional<Ty> get_nonrequired_option(
  option const &opt,
  boost::program_options::variables_map const &vm,
  errors_t &errors)
{
  if (vm.count(opt.full_name) == 0) {
    return std::nullopt;
  }

  try {
    return vm.at(opt.full_name).as<Ty>();
  } catch (std::exception const &except) {
    errors.emplace_back(make_str("%s %s", opt.to_string().c_str(), except.what()));
    return std::nullopt;
  }
}

b8 get_flag_option(
  option const &opt,
  boost::program_options::variables_map const &vm)
{
  return vm.count(opt.full_name) > 0;
}

static
std::string format_for_easy_init(option const &opt)
{
  return make_str("%s,%c", opt.full_name, opt.shorthand);
}

options_description po::make_image_options_description()
{
  options_description description("Options");

  auto const fmt = format_for_easy_init;

  using boost::program_options::value;
  using std::string;

  description.add_options()
    (fmt(make_image::out_file_path()).c_str(),
      value<string>(), "Output PGM file path.")

    (fmt(make_image::format()).c_str(),
      value<string>(), "PGM image format, raw|plain.")

    (fmt(make_image::content()).c_str(),
      value<string>(), make_str("Type of image content, /%s/.", make_image::regex_content()).c_str())

    (fmt(make_image::width()).c_str(),
      value<u16>(), make_str("Image width, [1, %zu].", UINT16_MAX).c_str())

    (fmt(make_image::height()).c_str(),
      value<u16>(), make_str("Image height, [1, %zu].", UINT16_MAX).c_str())

    (fmt(make_image::maxval()).c_str(),
      value<u16>(), make_str("Maximum pixel value, [1, %zu].", UINT8_MAX).c_str())
  ;

  return description;
}

options_description po::make_states_options_description()
{
  options_description description("Options");

  auto const fmt = format_for_easy_init;

  using boost::program_options::value;
  using std::string;

  description.add_options()
    (fmt(make_states::count()).c_str(),
      value<u64>(), "Number of randomized states to generate.")

    (fmt(make_states::out_dir_path()).c_str(),
      value<string>(), "Output directory for JSON state files.")

    (fmt(make_states::create_dirs()).c_str(),
      /* flag */ make_str( "Create --%s and parent directories if not present, off by default.",
                           make_states::out_dir_path().full_name ).c_str())

    (fmt(make_states::grid_width()).c_str(),
      value<i32>(), "Value of 'grid_width' for all generated states, [1, 65535].")

    (fmt(make_states::grid_height()).c_str(),
      value<i32>(), "Value of 'grid_height' for all generated states, [1, 65535].")

    (fmt(make_states::ant_col()).c_str(),
      value<i32>(), "Value of 'ant_col' for all generated states, [0, grid_width).")

    (fmt(make_states::ant_row()).c_str(),
      value<i32>(), "Value of 'ant_row' for all generated states, [0, grid_height).")

    (fmt(make_states::min_num_rules()).c_str(),
      value<u16>(), make_str( "Minimum number of rules for generated states, inclusive, default=%d.",
                              make_states::min_num_rules_default() ).c_str())

    (fmt(make_states::max_num_rules()).c_str(),
      value<u16>(), make_str( "Maximum number of rules for generated states, inclusive, default=%d.",
                              make_states::max_num_rules_default() ).c_str())

    (fmt(make_states::grid_state()).c_str(),
      value<string>(), make_str( "Value of 'grid_state' for all generated states, any string, default='%s'.",
                                 make_states::fill_string_default().c_str() ).c_str())

    (fmt(make_states::shade_order()).c_str(),
      value<string>(), make_str( "Ordering of rule shades, asc|desc|rand, default=%s",
                                 make_states::shade_order_default() ).c_str())

    (fmt(make_states::name_mode()).c_str(),
      value<string>(), make_str( "The method used for naming generated JSON state files, /%s/. "
                                 "'alpha,N' will generate a string of N random letters, e.g. 'aHCgt'. "
                                 "'turndirecs' will use the chain of turn directions, e.g. 'LRLN'. "
                                 "'randword,N' will use N random words from --word_file_path separated by underscores, e.g. 'w1_w2_w3'. ",
                                 make_states::name_mode_regex() ).c_str())

    (fmt(make_states::word_file_path()).c_str(),
      value<string>(), "Path of file whose content starts with a newline, followed by newline-separated words, and ends with a newline, e.g. '\\nW1\\nW2\\n'. "
                       "Only necessary when --name_mode is 'randwords,N'. ")

    (fmt(make_states::turn_directions()).c_str(),
      value<string>(), make_str( "Possible rule 'turn_dir' values, /%s/, default=%s. "
                                 "Values are chosen randomly from this list, so having repeat values makes them more likely to occur. "
                                 "For instance, 'LLLRRN' results in a 3/6 chance for L, 2/6 chance for R, and 1/6 chance for N. ",
                                 make_states::turn_directions_regex(), make_states::turn_directions_default() ).c_str())

    (fmt(make_states::ant_orientations()).c_str(),
      value<string>(), make_str( "Possible 'ant_orientation' values, /%s/, default=%s. "
                                 "Values are chosen randomly from this list, so having repeat values makes them more likely to occur. "
                                 "For instance, 'NNNEES' results in a 3/6 chance for N, 2/6 chance for E, 1/6 chance for S, and 0/6 chance for W. ",
                                 make_states::ant_orientations_regex(), make_states::ant_orientations_default() ).c_str())
  ;

  return description;
}

options_description simulation_options_description()
{
  options_description description("Simulation options");

  auto const fmt = format_for_easy_init;

  using boost::program_options::value;
  using std::string;

  description.add_options()
    (fmt(simulation::generation_limit()).c_str(),
      value<u64>(), "Generation limit, if reached the simulation will stop, 0 means max uint64.")

    (fmt(simulation::image_format()).c_str(),
      value<string>(), "PGM image format for saves, raw|plain.")

    (fmt(simulation::create_logs()).c_str(),
      /* flag */ "Create a log entry when a save is made.")

    (fmt(simulation::save_path()).c_str(),
      value<string>(), "Directory in which to save state JSON and PGM files.")

    (fmt(simulation::save_image_only()).c_str(),
      /* flag */ "Do not emit JSON files when saving state.")

    (fmt(simulation::save_final_state()).c_str(),
      /* flag */ "Ensures final state is saved regardless of save points or interval.")

    (fmt(simulation::save_points()).c_str(),
      value<string>(), "Specific generations (points) to save.")

    (fmt(simulation::save_interval()).c_str(),
      value<u64>(), "Generation interval at which to save.")
  ;

  return description;
}

options_description po::simulate_one_options_description()
{
  options_description description("General options");

  auto const fmt = format_for_easy_init;

  using boost::program_options::value;
  using std::string;

  description.add_options()
    (fmt(simulate_one::name()).c_str(),
      value<string>(), "Name of simulation, if unspecified state_file_path filename is used.")

    (fmt(simulate_one::state_file_path()).c_str(),
      value<string>(), "JSON file containing initial state.")

    (fmt(simulate_one::log_file_path()).c_str(),
      value<string>(), "Log file path.")
  ;

  description.add(simulation_options_description());

  return description;
}

options_description po::simulate_many_options_description()
{
  options_description description("General options");

  auto const fmt = format_for_easy_init;

  using boost::program_options::value;
  using std::string;

  description.add_options()
    (fmt(simulate_many::num_threads()).c_str(),
      value<u32>(), "Number of threads in thread pool.")

    (fmt(simulate_many::state_dir_path()).c_str(),
      value<string>(), "Path to directory containing initial JSON state files.")

    (fmt(simulate_many::log_file_path()).c_str(),
      value<string>(), "Log file path.")
  ;

  description.add(simulation_options_description());

  return description;
}

void po::parse_make_states_options(
  int const argc,
  char const *const *const argv,
  make_states_options &out,
  util::errors_t &errors)
{
  namespace bpo = boost::program_options;

  bpo::variables_map vm;

  try {
    bpo::store(
      bpo::parse_command_line(
        argc,
        argv,
        make_states_options_description()),
      vm);
  } catch (std::exception const &except) {
    errors.emplace_back(except.what());
    return;
  }

  bpo::notify(vm);

  {
    b8 const create_dirs = get_flag_option(make_states::create_dirs(), vm);
    out.create_dirs = create_dirs;
  }

  {
    option const opt = make_states::name_mode();
    auto name_mode = get_required_option<std::string>(opt, vm, errors);

    if (name_mode.has_value()) {
      if (std::regex_match(name_mode.value(), std::regex(make_states::name_mode_regex())))
        out.name_mode = std::move(name_mode.value());
      else
        errors.emplace_back(make_str("%s must match /%s/",
          opt.to_string().c_str(), make_states::name_mode_regex()));
    }
  }

  // TODO: add validation
  {
    auto grid_state = get_nonrequired_option<std::string>(make_states::grid_state(), vm, errors);

    if (grid_state.has_value()) {
      out.grid_state = std::move(grid_state.value());
    } else {
      out.grid_state = make_states::fill_string_default();
    }
  }

  {
    option const opt = make_states::turn_directions();
    auto turn_dirs = get_nonrequired_option<std::string>(opt, vm, errors);

    if (turn_dirs.has_value()) {
      if (!std::regex_match(turn_dirs.value(), std::regex(make_states::turn_directions_regex())))
        errors.emplace_back(make_str("%s must match /%s/",
          opt.to_string().c_str(), make_states::turn_directions_regex()));
      else
        out.turn_directions = std::move(turn_dirs.value());
    } else {
      out.turn_directions = make_states::turn_directions_default();
    }
  }

  {
    option const opt = make_states::shade_order();
    auto shade_order = get_nonrequired_option<std::string>(opt, vm, errors);

    if (shade_order.has_value()) {
      if (shade_order.value() == "asc" || shade_order.value() == "desc" || shade_order.value() == "rand")
        out.shade_order = std::move(shade_order.value());
      else
        errors.emplace_back(make_str("%s must be one of asc|desc|rand",
          opt.to_string().c_str()));
    } else {
      out.shade_order = make_states::shade_order_default();
    }
  }

  {
    option const opt = make_states::ant_orientations();
    auto ant_orients = get_nonrequired_option<std::string>(opt, vm, errors);

    if (ant_orients.has_value()) {
      if (!std::regex_match(ant_orients.value(), std::regex(make_states::ant_orientations_regex())))
        errors.emplace_back(make_str("%s must match /%s/",
          opt.to_string().c_str(), make_states::ant_orientations_regex()));
      else
        out.ant_orientations = std::move(ant_orients.value());
    } else {
      out.ant_orientations = make_states::ant_orientations_default();
    }
  }

  {
    option const opt = make_states::out_dir_path();
    auto out_dir_path = get_required_option<std::string>(opt, vm, errors);

    if (out_dir_path.has_value()) {
      if (!fs::exists(out_dir_path.value())) {
        if (out.create_dirs) {
          try {
            fs::create_directories(out_dir_path.value());
            out.out_dir_path = std::move(out_dir_path.value());
          } catch (fs::filesystem_error const &except) {
            print_err("unable to create directory: %s", except.what());
          }
        } else {
          errors.emplace_back(make_str("%s does not exist, specify %s to create it",
            opt.to_string().c_str(), make_states::create_dirs().to_string().c_str()));
        }
      } else if (!fs::is_directory(out_dir_path.value())) {
        errors.emplace_back(make_str("%s is not a directory", opt.to_string().c_str()));
      } else {
        out.out_dir_path = std::move(out_dir_path.value());
      }
    }
  }

  {
    option const opt = make_states::word_file_path();
    auto word_file_path = get_nonrequired_option<std::string>(opt, vm, errors);

    if (out.name_mode.starts_with("randwords") && !word_file_path.has_value()) {
      errors.emplace_back(make_str("%s required", opt.to_string().c_str()));
    } else if (word_file_path.has_value()) {
      if (!fs::is_regular_file(word_file_path.value()))
        errors.emplace_back(make_str("%s must be a regular file", opt.to_string().c_str()));
      else
        out.word_file_path = std::move(word_file_path.value());
    }
  }

  {
    option const opt = make_states::count();
    auto const count = get_required_option<usize>(opt, vm, errors);

    if (count.has_value()) {
      if (count.value() < 1) {
        errors.emplace_back(make_str("%s must be > 0",
          opt.to_string().c_str(), make_states::ant_orientations_regex()));
      } else {
        out.count = count.value();
      }
    }
  }

  {
    option const opt_min = make_states::min_num_rules();
    option const opt_max = make_states::max_num_rules();

    auto const min_num_rules = get_nonrequired_option<u16>(opt_min, vm, errors)
      .value_or(make_states::min_num_rules_default());
    auto const max_num_rules = get_nonrequired_option<u16>(opt_max, vm, errors)
      .value_or(make_states::max_num_rules_default());

    auto const in_range = [](u16 const n) { return n >= 2 && n <= 256; };

    b8 const min_in_allowed_range = in_range(min_num_rules);
    b8 const max_in_allowed_range = in_range(max_num_rules);

    if (!min_in_allowed_range || !max_in_allowed_range) {
      if (!min_in_allowed_range)
        errors.emplace_back(make_str("%s must be in range [2, 256]",
          opt_min.to_string().c_str()));
      if (!max_in_allowed_range)
        errors.emplace_back(make_str("%s must be in range [2, 256]",
          opt_max.to_string().c_str()));
    } else if (min_num_rules > max_num_rules) {
      errors.emplace_back(make_str("%s must be >= %s",
        opt_max.to_string().c_str(), opt_min.to_string().c_str()));
    } else {
      out.min_num_rules = min_num_rules;
      out.max_num_rules = max_num_rules;
    }
  }

  {
    option const opt_gw = make_states::grid_width();
    option const opt_ac = make_states::ant_col();

    auto const grid_width = get_required_option<i32>(opt_gw, vm, errors);
    auto const ant_col = get_required_option<i32>(opt_ac, vm, errors);

    if (grid_width.has_value() && ant_col.has_value()) {
      if (grid_width.value() < 1 || grid_width.value() > UINT16_MAX) {
        errors.emplace_back(make_str("%s must be in range [1, %zu]",
          opt_gw.to_string().c_str(), UINT16_MAX));
      } else if (!util::in_range_incl_excl<usize>(ant_col.value(), 0, grid_width.value())) {
        errors.emplace_back(make_str("%s must be on grid x-axis [0, %zu)",
          opt_ac.to_string().c_str(), grid_width.value()));
      } else {
        out.grid_width = grid_width.value();
        out.ant_col = ant_col.value();
      }
    }
  }

  {
    option const opt_gh = make_states::grid_height();
    option const opt_ar = make_states::ant_row();

    auto const grid_height = get_required_option<i32>(opt_gh, vm, errors);
    auto const ant_row = get_required_option<i32>(opt_ar, vm, errors);

    if (grid_height.has_value() && ant_row.has_value()) {
      if (grid_height.value() < 1 || grid_height.value() > UINT16_MAX) {
        errors.emplace_back(make_str("%s must be in range [1, %zu]",
          opt_gh.to_string().c_str(), UINT16_MAX));
      } else if (!util::in_range_incl_excl<usize>(ant_row.value(), 0, grid_height.value())) {
        errors.emplace_back(make_str("%s must be on grid y-axis [0, %zu)",
          opt_ar.to_string().c_str(), grid_height.value()));
      } else {
        out.grid_height = grid_height.value();
        out.ant_row = ant_row.value();
      }
    }
  }
}

void po::parse_make_image_options(
  int const argc,
  char const *const *const argv,
  make_image_options &out,
  util::errors_t &errors)
{
  namespace bpo = boost::program_options;

  bpo::variables_map vm;

  try {
    bpo::store(
      bpo::parse_command_line(
        argc,
        argv,
        make_image_options_description()),
      vm);
  } catch (std::exception const &except) {
    errors.emplace_back(except.what());
    return;
  }

  bpo::notify(vm);

  {
    option const opt = make_image::format();
    auto format = get_required_option<std::string>(opt, vm, errors);

    if (format.has_value()) {
      if (format.value() == "raw")
        out.format = pgm8::format::RAW;
      else if (format.value() == "plain")
        out.format = pgm8::format::PLAIN;
      else
        errors.emplace_back(make_str("%s must be one of raw|plain",
          opt.to_string().c_str()));
    }
  }

  {
    option const opt = make_image::out_file_path();
    auto out_file_path = get_required_option<std::string>(opt, vm, errors);

    if (out_file_path.has_value()) {
      if (!util::file_is_openable(out_file_path.value()))
        errors.emplace_back(make_str("failed to open %s", opt.to_string().c_str()));
      else
        out.out_file_path = std::move(out_file_path.value());
    }
  }

  {
    option const opt = make_image::content();
    auto content = get_required_option<std::string>(opt, vm, errors);

    if (content.has_value()) {
      if (std::regex_match(content.value(), std::regex(make_image::regex_content()))) {
        if (content.value().starts_with("fill")) {
          usize const equals_sign_pos = content.value().find_first_of('=');
          std::string const fill_val_str = content.value().substr(equals_sign_pos + 1);
          usize const fill_val = std::stoull(fill_val_str);
          if (fill_val > UINT8_MAX) {
            errors.emplace_back(make_str("%s fill value must be <= %zu",
              opt.to_string().c_str(), UINT8_MAX));
          } else {
            out.fill_value = static_cast<i16>(fill_val);
          }
        } else {
          out.fill_value = -1;
        }
        out.content = std::move(content.value());
      } else {
        errors.emplace_back(make_str("%s must match /%s/",
          opt.to_string().c_str(), make_image::regex_content()));
      }
    }
  }

  {
    option const opt = make_image::width();
    auto const width = get_required_option<u16>(opt, vm, errors);

    if (width.has_value()) {
      if (width.value() < 1)
        errors.emplace_back(make_str("%s must be in range [1, %zu]",
          opt.to_string().c_str(), UINT16_MAX));
      else
        out.width = width.value();
    }
  }

  {
    option const opt = make_image::height();
    auto const height = get_required_option<u16>(opt, vm, errors);

    if (height.has_value()) {
      if (height.value() < 1)
        errors.emplace_back(make_str("%s must be in range [1, %zu]",
          opt.to_string().c_str(), UINT16_MAX));
      else
        out.height = height.value();
    }
  }

  {
    option const opt = make_image::maxval();
    auto const maxval = get_required_option<u16>(opt, vm, errors);

    if (maxval.has_value()) {
      if (maxval.value() < 1 || maxval.value() > UINT8_MAX)
        errors.emplace_back(make_str("%s must be in range [1, %zu]",
          opt.to_string().c_str(), UINT8_MAX));
      else
        out.maxval = static_cast<u8>(maxval.value());
    }
  }
}

static
void validate_and_set_simulation_options(
  po::simulation_options &out,
  boost::program_options::variables_map const &vm,
  errors_t &errors)
{
  {
    option const opt = simulation::save_points();
    auto const save_points = get_nonrequired_option<std::string>(opt, vm, errors);

    if (save_points.has_value()) {
      try {
        out.save_points = util::parse_json_array_u64(save_points.value().c_str());
      } catch (json_t::parse_error const &) {
        errors.emplace_back(make_str("%s must be a JSON array",
          opt.to_string().c_str()));
      } catch (std::runtime_error const &except) {
        errors.emplace_back(make_str("%s %s",
          opt.to_string().c_str(), except.what()));
      }
    } else {
      out.save_points = {};
    }
  }

  {
    option const opt = simulation::generation_limit();
    auto const generation_limit = get_required_option<u64>(opt, vm, errors);

    if (generation_limit.has_value())
      out.generation_limit = generation_limit.value();
  }

  {
    auto const save_interval = get_nonrequired_option<u64>(simulation::save_interval(), vm, errors);
    out.save_interval = save_interval.value_or(0);
  }

  {
    b8 const save_final_state = get_flag_option(simulation::save_final_state(), vm);
    out.save_final_state = save_final_state;
  }

  {
    b8 const create_logs = get_flag_option(simulation::create_logs(), vm);
    out.create_logs = create_logs;
  }

  {
    b8 const save_image_only = get_flag_option(simulation::save_image_only(), vm);
    out.save_image_only = save_image_only;
  }

  {
    option const opt = simulation::save_path();
    b8 const save_trigger_present = out.save_final_state || out.save_interval || !out.save_points.empty();
    auto save_path = save_trigger_present
      ? get_required_option<std::string>(opt, vm, errors)
      : get_nonrequired_option<std::string>(opt, vm, errors);

    if (save_path.has_value()) {
      if (!fs::is_directory(save_path.value()))
        errors.emplace_back(make_str("%s not a directory", opt.to_string().c_str()));
      else
        out.save_path = std::move(save_path.value());
    }
  }

  {
    option const opt = simulation::image_format();
    auto const image_format = get_nonrequired_option<std::string>(opt, vm, errors);

    if (image_format.has_value()) {
      if (image_format.value() == "raw")
        out.image_format = pgm8::format::RAW;
      else if (image_format.value() == "plain")
        out.image_format = pgm8::format::PLAIN;
      else
        errors.emplace_back(make_str("%s must be one of raw|plain",
          opt.to_string().c_str()));
    } else {
      out.image_format = pgm8::format::RAW;
    }
  }
}

void po::parse_simulate_one_options(
  int const argc,
  char const *const *const argv,
  simulate_one_options &out,
  util::errors_t &errors)
{
  namespace bpo = boost::program_options;

  bpo::variables_map vm;

  try {
    bpo::store(
      bpo::parse_command_line(
        argc,
        argv,
        simulate_one_options_description()),
      vm);
  } catch (std::exception const &except) {
    errors.emplace_back(except.what());
    return;
  }

  bpo::notify(vm);

  validate_and_set_simulation_options(out.sim, vm, errors);

  {
    option const opt = simulate_one::name();
    auto name = get_nonrequired_option<std::string>(opt, vm, errors);

    if (name.has_value()) {
      char const *const name_regex_cstr = "^[a-zA-Z0-9-_]{1,}$";
      if (!std::regex_match(name.value(), std::regex(name_regex_cstr))) {
        errors.emplace_back(make_str("%s doesn't match /%s/",
          opt.to_string().c_str(), name_regex_cstr).c_str());
      } else {
        out.name = std::move(name.value());
      }
    } else {
      out.name = "";
    }
  }

  {
    option const opt = simulate_one::state_file_path();
    auto state_file_path = get_required_option<std::string>(opt, vm, errors);

    if (state_file_path.has_value()) {
      if (!fs::is_regular_file(state_file_path.value()))
        errors.emplace_back(make_str("%s must be a regular file", opt.to_string().c_str()));
      else {
        if (!util::file_is_openable(state_file_path.value()))
          errors.emplace_back(make_str("failed to open %s", opt.to_string().c_str()));
        else
          out.state_file_path = std::move(state_file_path.value());
      }
    }
  }

  {
    option const opt = simulate_one::log_file_path();
    auto log_file_path = out.sim.create_logs
      ? get_required_option<std::string>(opt, vm, errors)
      : get_nonrequired_option<std::string>(opt, vm, errors);

    if (log_file_path.has_value()) {
      if (!util::file_is_openable(log_file_path.value()))
        errors.emplace_back(make_str("failed to open %s", opt.to_string().c_str()));
      else
        out.log_file_path = std::move(log_file_path.value());
    } else {
      out.log_file_path = "";
    }
  }
}

void po::parse_simulate_many_options(
  int const argc,
  char const *const *const argv,
  simulate_many_options &out,
  util::errors_t &errors)
{
  namespace bpo = boost::program_options;

  bpo::variables_map vm;

  try {
    bpo::store(
      bpo::parse_command_line(
        argc,
        argv,
        simulate_many_options_description()),
      vm);
  } catch (std::exception const &except) {
    errors.emplace_back(except.what());
    return;
  }

  bpo::notify(vm);

  validate_and_set_simulation_options(out.sim, vm, errors);

  {
    option const opt = simulate_many::state_dir_path();
    auto state_dir_path = get_required_option<std::string>(opt, vm, errors);

    if (state_dir_path.has_value()) {
      if (!fs::is_directory(state_dir_path.value()))
        errors.emplace_back(make_str("%s is not a directory", opt.to_string().c_str()));
      else
        out.state_dir_path = std::move(state_dir_path.value());
    }
  }

  {
    option const opt = simulate_one::log_file_path();
    auto log_file_path = out.sim.create_logs
      ? get_required_option<std::string>(opt, vm, errors)
      : get_nonrequired_option<std::string>(opt, vm, errors);

    if (log_file_path.has_value()) {
      if (!util::file_is_openable(log_file_path.value()))
        errors.emplace_back(make_str("failed to open %s", opt.to_string().c_str()));
      else
        out.log_file_path = std::move(log_file_path.value());
    } else {
      out.log_file_path = "";
    }
  }

  {
    option const opt = simulate_many::num_threads();
    auto num_threads = get_nonrequired_option<u32>(simulate_many::num_threads(), vm, errors);

    if (num_threads.has_value()) {
      if (num_threads.value() == 0)
        errors.emplace_back(make_str("%s must be > 0",
          opt.to_string().c_str()));
      else
        out.num_threads = num_threads.value();
    } else {
      out.num_threads = std::max(std::thread::hardware_concurrency(), 1ui32);
    }
  }
}
