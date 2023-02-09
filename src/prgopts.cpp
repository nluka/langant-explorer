#include "exit.hpp"
#include "prgopts.hpp"
#include "types.hpp"

namespace bpo = boost::program_options;

char const *prgopts::usage_msg() {
  return
    "USAGE\n"
    "  <name> <scenario> <gentarget>\n"
    "  [img_fmt] [save_points] [save_interval]\n"
    "REQUIRED\n"
    "  name ............ name of simuation\n"
    "  scenario ........ simulation JSON file path\n"
    "  gentarget ....... uint64, 0 meaning no target\n"
    "OPTIONAL\n"
    "  imgfmt .......... rawPGM|plainPGM, default = rawPGM\n"
    "  savefinalstate .. flag, guarantees final state is saved\n"
    "  savepoints ...... generations to save, syntax -> [1,2,...]\n"
    "  saveinterval .... non-0 uint64\n"
  ;
}

bpo::options_description prgopts::options_descrip() {
  bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()
    ("name", bpo::value<std::string>(), empty_desc)
    ("scenario", bpo::value<std::string>(), empty_desc)
    ("gentarget", bpo::value<u64>(), empty_desc)
    ("imgfmt", bpo::value<std::string>(), empty_desc)
    ("savefinalstate", /* flag */ empty_desc)
    ("savepoints", bpo::value<std::string>(), empty_desc)
    ("saveinterval", bpo::value<u64>(), empty_desc)
  ;

  return desc;
}
