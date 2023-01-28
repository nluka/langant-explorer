#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "types.hpp"

#ifdef _MSC_VER
  #define MICROSOFT_COMPILER 1
#elif __GNUC__
  #define GXX_COMPILER 1
#endif

#ifdef MICROSOFT_COMPILER
#include <cstdlib>
#endif

namespace util {

template <typename Ty>
// returns true if `val` is in range [min, max), false otherwise.
bool in_range_incl_excl(Ty const val, Ty const min, Ty const max) {
  return val >= min && val < max;
}

std::fstream open_file(char const *pathname, int flags);
std::string extract_txt_file_contents(char const *pathname);

// Returns the size of a static (stack-allocated) C-style array at compile time.
template <typename ElemTy, usize Length>
consteval
usize lengthof(ElemTy (&)[Length]) {
  // implementation from: https://stackoverflow.com/a/2404697/16471560
  return Length;
}

// The .what() string of a nlohmann::json exception has the format:
// "[json.exception.x.y] explanation" where x is a class name and y is a positive integer.
//                       ^ This function extracts a string starting from exactly this point.
template <typename Ty>
char const *nlohmann_json_extract_sentence(Ty const &except) {
  auto const from_first_space = std::strchr(except.what(), ' ');
  if (from_first_space == nullptr) {
    throw std::runtime_error("unable to find first space in nlohmann.json exception string");
  } else {
    return from_first_space + 1; // trim off leading space
  }
}

} // namespace util

#endif // UTIL_HPP
