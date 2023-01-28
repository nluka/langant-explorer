#include "exit.hpp"
#include "prgopts.hpp"
#include "types.hpp"

namespace bpo = boost::program_options;

char const *prgopts::usage_msg() {
  return
    "USAGE\n"
    "  <name> <scenario> <gentarget> [<img_fmt>]\n"
    "REQUIRED\n"
    "  <name> ------> name of simuation\n"
    "  <scenario> --> simulation JSON file path\n"
    "  <gentarget> -> uint64, 0 meaning no target\n"
    "OPTIONS\n"
    "  <imgfmt> ----> rawPGM|plainPGM, default = rawPGM\n"
  ;
}

bpo::options_description prgopts::options_descrip() {
  static bpo::options_description desc("OPTIONS");
  char const *const empty_desc = "";

  desc.add_options()
    ("name", bpo::value<std::string>(), empty_desc)
    ("scenario", bpo::value<std::string>(), empty_desc)
    ("gentarget", bpo::value<usize>(), empty_desc)
    ("imgfmt", bpo::value<std::string>(), empty_desc)
    ("savefinalstate", empty_desc)
  ;

  return desc;
}
