#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#ifdef _MSC_VER
  #define MICROSOFT_COMPILER 1
#elif __GNUC__
  #define GXX_COMPILER 1
#endif

#ifdef MICROSOFT_COMPILER
#include <cstdlib>
#endif

namespace util {

uint16_t byteswap_uint16(uint16_t val);
uint32_t byteswap_uint32(uint32_t val);

std::fstream open_file(char const *pathname, int flags);
std::vector<char> extract_bin_file_contents(char const *pathname);
std::string extract_txt_file_contents(char const *pathname);

// Returns the size of a static (stack-allocated) C-style array at compile time.
template <typename ElemTy, size_t Length>
consteval
size_t lengthof(ElemTy (&)[Length]) {
  // implementation from: https://stackoverflow.com/a/2404697/16471560
  return Length;
}

template <typename ElemTy>
bool vector_cmp(std::vector<ElemTy> const &a, std::vector<ElemTy> const &b) {
  if (a.size() != b.size())
    return false;

  for (size_t i = 0; i < a.size(); ++i)
    if (a[i] != b[i])
      return false;

  return true;
}

std::string format_file_size(uintmax_t size);
void format_file_size(uintmax_t size, char *out, size_t outSize);

} // namespace util

#endif // UTIL_HPP
