#ifndef UTIL_HPP
#define UTIL_HPP

#pragma warning(push, 0)
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#pragma warning(pop)

#include "primitives.hpp"

namespace util {

   typedef std::vector<std::string> strvec_t;

   std::string make_str(char const *const fmt, ...);

   template <typename Ty>
   // returns true if `val` is in range [min, max), false otherwise.
   b8 in_range_incl_excl(Ty const val, Ty const min, Ty const max)
   {
      return val >= min && val < max;
   }

   std::fstream open_file(char const *pathname, int flags);
   std::string extract_txt_file_contents(char const *pathname);

   // Returns the size of a static (stack-allocated) C-style array at compile time.
   template <typename ElemTy, usize Length>
   consteval usize lengthof(ElemTy (&)[Length])
   {
      return Length;
   }

   // The .what() string of a nlohmann::json exception has the format:
   // "[json.exception.x.y] explanation" where x is a class name and y is a positive integer.
   // This function extracts the explanation component.
   template <typename Ty>
   char const *nlohmann_json_extract_sentence(Ty const &except)
   {
      auto const from_first_space = std::strchr(except.what(), ' ');
      if (from_first_space == nullptr) {
         throw std::runtime_error("unable to find first space in nlohmann.json exception string");
      } else {
         return from_first_space + 1; // trim off leading space
      }
   }

   std::vector<u64> parse_json_array_u64(char const *str);

} // namespace util

#endif // UTIL_HPP
