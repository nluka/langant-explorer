#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <chrono>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

#include <boost/program_options.hpp>

#include "primitives.hpp"

namespace util
{
   typedef std::vector<std::string> errors_t;
   typedef std::chrono::steady_clock::time_point time_point_t;

   [[nodiscard]] time_point_t current_time();

   [[nodiscard]] std::chrono::nanoseconds nanos_between(time_point_t a, time_point_t b);

   [[nodiscard]] std::string make_str(char const *fmt, ...);

   int print_err(char const *fmt, ...);

   [[noreturn]] void die(char const *fmt, ...);

   [[nodiscard]] std::fstream open_file(char const *path, std::ios_base::openmode flags);
   [[nodiscard]] std::fstream open_file(std::string const &path, std::ios_base::openmode flags);

   [[nodiscard]] bool get_user_choice(std::string const &prompt);

   [[nodiscard]] std::vector<u64> parse_json_array_u64(char const *str);

   template <typename IntTy>
   constexpr
   [[nodiscard]] usize count_digits(IntTy n)
   {
      if (n == 0)
         return 1;

      usize count = 0;
      while (n != 0) {
         n = n / 10;
         ++count;
      }

      return count;
   }

   template <typename PathTy>
   [[nodiscard]] std::string extract_txt_file_contents(
      PathTy const &path,
      bool const normalize_newlines)
   {
      std::fstream file = util::open_file(path, std::ios::in);
      auto const file_size = std::filesystem::file_size(path);

      std::string content{};
      content.reserve(file_size);

      getline(file, content, '\0');

      if (normalize_newlines) {
         // remove any \r characters
         content.erase(remove(content.begin(), content.end(), '\r'), content.end());
      }

      return content;
   }

   // returns true if `val` is in range [min, max), false otherwise.
   template <typename Ty>
   [[nodiscard]] b8 in_range_incl_excl(Ty const val, Ty const min, Ty const max)
   {
      return val >= min && val < max;
   }

   template <typename Ty>
   [[nodiscard]] std::optional<Ty> get_required_option(
      char const *const full_name,
      char const *const short_name,
      boost::program_options::variables_map const &var_map,
      errors_t &errors)
   {
      if (var_map.count(full_name) == 0) {
         errors.emplace_back(make_str("(--%s, -%s) required option missing", full_name, short_name));
         return std::nullopt;
      }

      try {
         return var_map.at(full_name).as<Ty>();
      } catch (...) {
         errors.emplace_back(make_str("(--%s, -%s) unable to parse value", full_name, short_name));
         return std::nullopt;
      }
   }

   template <typename Ty>
   [[nodiscard]] std::optional<Ty> get_nonrequired_option(
      char const *const full_name,
      char const *const short_name,
      boost::program_options::variables_map const &var_map,
      errors_t &errors)
   {
      if (var_map.count(full_name) == 0) {
         return std::nullopt;
      }

      try {
         return var_map.at(full_name).as<Ty>();
      } catch (...) {
         errors.emplace_back(make_str("(--%s , -%s) unable to parse value", full_name, short_name));
         return std::nullopt;
      }
   }

   inline
   [[nodiscard]] b8 get_flag_option(
      char const *const option_name,
      boost::program_options::variables_map const &var_map)
   {
      return var_map.count(option_name) > 0;
   }

   // Returns the size of a static (stack-allocated) C-style array at compile time.
   template <typename ElemTy, usize Length>
   [[nodiscard]] consteval usize lengthof(ElemTy (&)[Length])
   {
      return Length;
   }

   // The .what() string of a nlohmann::json exception has the format:
   // "[json.exception.x.y] explanation" where x is a class name and y is a positive integer.
   // This function extracts the explanation component.
   template <typename Ty>
   [[nodiscard]] char const *nlohmann_json_extract_sentence(Ty const &except)
   {
      auto const from_first_space = std::strchr(except.what(), ' ');
      if (from_first_space == nullptr) {
         throw std::runtime_error("unable to find first space in nlohmann.json exception string");
      } else {
         return from_first_space + 1; // trim off leading space
      }
   }

} // namespace util

#endif // UTIL_HPP
