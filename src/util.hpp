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

u16 byteswap_u16(u16 val);
u32 byteswap_u32(u32 val);

std::fstream open_file(char const *pathname, int flags);
std::vector<char> extract_bin_file_contents(char const *pathname);
std::string extract_txt_file_contents(char const *pathname);

// Returns the size of a static (stack-allocated) C-style array at compile time.
template <typename ElemTy, usize Length>
consteval
usize lengthof(ElemTy (&)[Length]) {
  // implementation from: https://stackoverflow.com/a/2404697/16471560
  return Length;
}

template <typename ElemTy>
bool vector_cmp(std::vector<ElemTy> const &a, std::vector<ElemTy> const &b) {
  if (a.size() != b.size())
    return false;

  for (usize i = 0; i < a.size(); ++i)
    if (a[i] != b[i])
      return false;

  return true;
}

std::string format_file_size(usize size);
void format_file_size(usize size, char *out, usize out_size);

} // namespace util

#endif // UTIL_HPP
