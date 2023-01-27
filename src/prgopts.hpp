#ifndef PRGOPTS_HPP
#define PRGOPTS_HPP

#include <boost/program_options.hpp>

namespace prgopts {

char const *usage_msg();

boost::program_options::options_description options_descrip();

} // namespace prgopts

#endif // PRGOPTS_HPP
