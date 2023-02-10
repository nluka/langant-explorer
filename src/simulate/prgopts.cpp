#include "../primitives.hpp"
#include "exit.hpp"
#include "prgopts.hpp"

namespace bpo = boost::program_options;

char const *prgopts::usage_msg()
{
  return
    "USAGE\n"
    "  simulate <name> <cfg> <maxgen>\n"
    "  [img_fmt] [save_points] [save_interval]\n"
    "REQUIRED\n"
    "  name ............ name of simuation\n"
    "  cfg ............. JSON file path to initial state JSON\n"
    "  maxgen .......... maximum generation, uint64, 0 = no maximum\n"
    "OPTIONAL\n"
    "  imgfmt .......... rawPGM|plainPGM, default = rawPGM\n"
    "  savefinalstate .. flag, guarantees final state is saved\n"
    "  savepoints ...... generations to save, syntax -> [1,2,...]\n"
    "  saveinterval .... non-0 uint64\n"
  ;
}

bpo::options_description prgopts::options_descrip()
{
  bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()(
    "name", bpo::value<std::string>(), empty_desc
  )(
    "cfg", bpo::value<std::string>(), empty_desc
  )(
    "maxgen", bpo::value<u64>(), empty_desc
  )(
    "imgfmt", bpo::value<std::string>(), empty_desc
  )(
    "savefinalstate", /* flag */ empty_desc
  )(
    "savepoints", bpo::value<std::string>(), empty_desc
  )(
    "saveinterval", bpo::value<u64>(), empty_desc
  );

  // TODO: add --savedir option

  return desc;
}
