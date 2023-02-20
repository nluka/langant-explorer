#include <boost/program_options.hpp>
#include "lib/json.hpp"
#include "lib/term.hpp"

#include "util.hpp"
#include "simulation.hpp"

namespace bpo = boost::program_options;
namespace fs = std::filesystem;
using util::errors_t;

bpo::options_description simulation::env_options_descrip(char const *const state_path_desc)
{
  bpo::options_description desc("SIMULATION OPTIONS");

  desc.add_options()
    (SIM_OPT_STATEPATH_FULL "," SIM_OPT_STATEPATH_SHORT, bpo::value<std::string>(), state_path_desc)
    (SIM_OPT_GENLIM_FULL "," SIM_OPT_GENLIM_SHORT, bpo::value<u64>(), "*** generation limit, if reached the simulation will stop, > 0 uint64")
    (SIM_OPT_IMGFMT_FULL "," SIM_OPT_IMGFMT_SHORT, bpo::value<std::string>(), "type of PGM image to produce on save, rawPGM|plainPGM, default=rawPGM")
    (SIM_OPT_SAVEFINALSTATE_FULL "," SIM_OPT_SAVEFINALSTATE_SHORT, "flag, if set will ensure final generation is saved regardless of any save points or interval")
    (SIM_OPT_SAVEPOINTS_FULL "," SIM_OPT_SAVEPOINTS_SHORT, bpo::value<std::string>(), "specific generations to save, JSON uint64 array")
    (SIM_OPT_SAVEINTERVAL_FULL "," SIM_OPT_SAVEINTERVAL_SHORT, bpo::value<u64>(), "generation interval to save at, > 0 uint64")
    (SIM_OPT_SAVEPATH_FULL "," SIM_OPT_SAVEPATH_SHORT, bpo::value<std::string>(), "*** directory in which to save state .json and .pgm files")
  ;

  return desc;
}

std::variant<
  simulation::env_config,
  errors_t
> simulation::extract_env_config(
  int const argc,
  char const *const *const argv,
  char const *const state_path_desc)
{
  using namespace term::color;
  using json_t = nlohmann::json;
  using util::get_required_option;
  using util::get_nonrequired_option;
  using util::get_flag_option;
  using util::make_str;

  simulation::env_config cfg{};
  errors_t errors{};

  bpo::variables_map var_map;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, env_options_descrip(state_path_desc)), var_map);
  } catch (std::exception const &err) {
    errors.emplace_back(err.what());
    return errors;
  }
  bpo::notify(var_map);

  {
    auto state_path = get_required_option<std::string>(SIM_OPT_STATEPATH_FULL, SIM_OPT_STATEPATH_SHORT, var_map, errors);

    if (state_path.has_value()) {
      if (!fs::exists(state_path.value())) {
        errors.emplace_back("(--" SIM_OPT_STATEPATH_FULL ", -" SIM_OPT_STATEPATH_SHORT ") path not found");
      } else {
        cfg.state_path = std::move(state_path.value());
      }
    }
  }

  {
    auto const generation_limit = get_required_option<u64>(SIM_OPT_GENLIM_FULL, SIM_OPT_GENLIM_SHORT, var_map, errors);

    if (generation_limit.has_value()) {
      if (generation_limit.value() == 0) {
        errors.emplace_back("(--" SIM_OPT_GENLIM_FULL ", -" SIM_OPT_GENLIM_SHORT ") must be > 0");
      } else {
        cfg.generation_limit = generation_limit.value();
      }
    }
  }

  {
    auto const img_fmt = get_nonrequired_option<std::string>(SIM_OPT_IMGFMT_FULL, var_map, errors);

    if (img_fmt.has_value()) {
      if (img_fmt.value() == "rawPGM") {
        cfg.img_fmt = pgm8::format::RAW;
      } else if (img_fmt.value() == "plainPGM") {
        cfg.img_fmt = pgm8::format::PLAIN;
      } else {
        errors.emplace_back("(--" SIM_OPT_IMGFMT_FULL ", -" SIM_OPT_IMGFMT_SHORT ") unrecognized value");
      }
    } else {
      cfg.img_fmt = pgm8::format::RAW;
    }
  }

  {
    auto const save_points = get_nonrequired_option<std::string>(SIM_OPT_SAVEPOINTS_FULL, var_map, errors);

    if (save_points.has_value()) {
      try {
        cfg.save_points = util::parse_json_array_u64(save_points.value().c_str());
      } catch (json_t::parse_error const &) {
        errors.emplace_back("(--" SIM_OPT_SAVEPOINTS_FULL ", -" SIM_OPT_SAVEPOINTS_SHORT ") unable to parse as JSON array");
      } catch (std::runtime_error const &except) {
        errors.emplace_back(make_str("(--" SIM_OPT_SAVEPOINTS_FULL ", -" SIM_OPT_SAVEPOINTS_SHORT ") unable to parse, %s", except.what()));
      }
    } else {
      cfg.save_points = {};
    }
  }

  {
    auto save_path = get_required_option<std::string>(SIM_OPT_SAVEPATH_FULL, SIM_OPT_SAVEPATH_SHORT, var_map, errors);

    if (save_path.has_value()) {
      if (!fs::exists(save_path.value())) {
        errors.emplace_back("(--" SIM_OPT_SAVEPATH_FULL ", -" SIM_OPT_SAVEPATH_SHORT ") path not found");
      } else {
        cfg.save_path = std::move(save_path.value());
      }
    }
  }

  {
    auto const save_interval = get_nonrequired_option<u64>(SIM_OPT_SAVEINTERVAL_FULL, var_map, errors);

    if (save_interval.has_value()) {
      cfg.save_interval = save_interval.value();
    } else {
      cfg.save_interval = 0;
    }
  }

  {
    b8 const save_final_state = get_flag_option(SIM_OPT_SAVEFINALSTATE_FULL, var_map);
    cfg.save_final_state = save_final_state;
  }

  if (errors.empty()) {
    return std::move(cfg);
  } else {
    return std::move(errors);
  }
}
