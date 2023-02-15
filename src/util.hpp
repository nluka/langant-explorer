#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

#include "primitives.hpp"

namespace util
{
   typedef std::vector<std::string> strvec_t;
   typedef std::chrono::steady_clock::time_point time_point_t;

   template <typename ExitCodeTy>
   [[noreturn]]
   void die(ExitCodeTy const code)
   {
      std::exit(static_cast<int>(code));
   }

   [[nodiscard]]
   time_point_t current_time();

   [[nodiscard]]
   std::chrono::nanoseconds nanos_between(time_point_t a, time_point_t b);

   [[nodiscard]]
   std::string make_str(char const *const fmt, ...);

   // returns true if `val` is in range [min, max), false otherwise.
   template <typename Ty>
   [[nodiscard]]
   b8 in_range_incl_excl(Ty const val, Ty const min, Ty const max)
   {
      return val >= min && val < max;
   }

   [[nodiscard]]
   std::fstream open_file(char const *path, int flags);

   [[nodiscard]]
   std::string extract_txt_file_contents(char const *path, bool const normalize_newlines);

   // Returns the size of a static (stack-allocated) C-style array at compile time.
   template <typename ElemTy, usize Length>
   [[nodiscard]]
   consteval usize lengthof(ElemTy (&)[Length])
   {
      return Length;
   }

   // The .what() string of a nlohmann::json exception has the format:
   // "[json.exception.x.y] explanation" where x is a class name and y is a positive integer.
   // This function extracts the explanation component.
   template <typename Ty>
   [[nodiscard]]
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
