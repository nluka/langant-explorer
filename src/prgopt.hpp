#ifndef CLARG_HPP
#define CLARG_HPP

#include <iostream>
#include <optional>

#include <boost/program_options.hpp>
#include "lib/term.hpp"

#include "primitives.hpp"
#include "util.hpp"

namespace prgopt
{
  template <typename Ty>
  Ty get_required_arg(
    char const *const argname,
    boost::program_options::variables_map const &args,
    int const exit_code)
  {
    if (args.count(argname) == 0) {
      using namespace term::color;
      printf(fore::RED | back::BLACK, "fatal: missing required option --%s", argname);
      util::die(exit_code);
    } else {
      return args.at(argname).as<Ty>();
    }
  }

  template <typename Ty>
  std::optional<Ty> get_optional_arg(
    char const *const argname,
    boost::program_options::variables_map const &args)
  {
    if (args.count(argname) == 0) {
      return std::nullopt;
    } else {
      return args.at(argname).as<Ty>();
    }
  }

  inline
  b8 get_flag_arg(
    char const *const argname,
    boost::program_options::variables_map const &args)
  {
    return args.count(argname) > 0;
  }

} // namespace clarg

#endif // CLARG_HPP
