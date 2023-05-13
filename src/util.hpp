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
#include <thread>

#include <boost/program_options.hpp>

#include "primitives.hpp"

namespace util
{
  template <typename Ty>
  using ascending_order = std::less<Ty>;

  typedef std::vector<std::string> errors_t;

  struct time_span
  {
    time_span(u64 seconds_elapsed) noexcept;

    char *stringify(char *out, u64 out_len) const;
    std::string to_string() const;

    private:
      u64 days;
      u64 hours;
      u64 minutes;
      u64 seconds;
  };

  typedef std::chrono::high_resolution_clock::time_point time_point_t;
  time_point_t current_time();
  u64 nanos_between(time_point_t start, time_point_t end);

  std::string make_str(char const *fmt, ...);

  i32 print_err(char const *fmt, ...);
  std::string stringify_errors(errors_t const &);

  [[noreturn]] void die(char const *fmt, ...);

  std::fstream open_file(char const *path, std::ios_base::openmode flags);
  std::fstream open_file(std::string const &path, std::ios_base::openmode flags);
  b8 file_is_openable(std::string const &path);

  b8 get_user_choice(std::string const &prompt);

  std::vector<u64> parse_json_array_u64(char const *str);

  void set_thread_priority_high(std::thread &thr);

  template <typename IntTy>
  constexpr
  u8 count_digits(IntTy n)
  {
    // who knows, maybe there will be 128 or 256 bit integers in the future...
    static_assert(sizeof(IntTy) <= 8);

    if (n == 0)
      return 1;

    u8 count = 0;
    while (n != 0) {
      n = n / 10;
      ++count;
    }

    return count;
  }

  template <typename PathTy>
  std::string extract_txt_file_contents(
    PathTy const &path,
    b8 const normalize_newlines)
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
  b8 in_range_incl_excl(Ty const val, Ty const min, Ty const max)
  {
    return val >= min && val < max;
  }

  // Converts an ASCII digit ('0'-'9') to an integer value (0-9).
  template <typename Ty>
  Ty ascii_digit_to(char const ch)
  {
    assert(ch >= '0' && ch <= '9');
    return static_cast<Ty>(ch) - 48;
  }

  // Returns the size of a static (stack-allocated) C-style array at compile time.
  template <typename ElemTy, u64 Length>
  consteval u64 lengthof(ElemTy (&)[Length])
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

} // namespace util

#endif // UTIL_HPP
