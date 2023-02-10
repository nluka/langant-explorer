#ifndef NLUKA_NTEST_HPP
#define NLUKA_NTEST_HPP

#pragma warning(push, 0)
#include <array>
#include <concepts>
#include <fstream>
#include <functional>
#include <source_location>
#include <sstream>
#include <string>
#include <type_traits>
#pragma warning(pop)

namespace ntest {

namespace config {

  void set_output_path(char const *);

  void set_max_str_preview_len(size_t);

  void set_max_arr_preview_len(size_t);

} // namespace config

namespace concepts {

  template <typename Ty>
  concept comparable_neq = requires(Ty a, Ty b)
  {
    a != b;
  };

  template <typename Ty>
  concept printable = requires(std::ostream os, Ty obj)
  {
    os << obj;
  };

  template <typename Ty>
  concept derives_from_std_exception = requires(Ty) {
    std::derived_from<std::exception, Ty>;
  };

} // namespace concepts

namespace internal {

  char const *const preview_style();

  size_t max_str_preview_len();

  size_t max_arr_preview_len();

  std::string generate_file_pathname(std::source_location const &, char const *extension);

  void throw_if_file_not_open(std::fstream const &, char const *pathname);

  struct assertion
  {
    // for passed assertions: "type | expected"
    // for failed assertions: "type | expected | actual"
    std::string serialized_vals;
    std::source_location loc;
  };

  void register_passed_assertion(std::stringstream &&, std::source_location const &);

  void register_failed_assertion(std::stringstream &&, std::source_location const &);

  template <typename Ty>
  requires std::integral<Ty>
  auto normalize_integral_type(auto const val)
  {
    if constexpr (std::is_unsigned_v<Ty>)
      return static_cast<uintmax_t>(val);
    else
      return static_cast<intmax_t>(val);
  }

  template <typename Ty>
  requires concepts::printable<Ty>
  void write_arr_to_file(
    std::string const &pathname,
    Ty const *const arr,
    size_t const size)
  {
    std::fstream file(pathname, std::ios::out);
    ntest::internal::throw_if_file_not_open(file, pathname.c_str());

    for (size_t i = 0; i < size; ++i)
    {
      if constexpr (std::is_integral_v<Ty>)
      {
        // some integrals like uint8_t are treated strangely by ostream insertion,
        // so casting must be done
        if constexpr (std::is_unsigned_v<Ty>)
          file << static_cast<uintmax_t>(arr[i]);
        else
          file << static_cast<intmax_t>(arr[i]);
      }
      else
      {
        file << arr[i];
      }
      file << '\n';
    }
  }

  template <typename Ty>
  requires concepts::comparable_neq<Ty>
  bool arr_eq(
    Ty const *const a1,
    size_t const a1_size,
    Ty const *const a2,
    size_t const a2_size)
  {
    if (a1_size != a2_size)
      return false;

    for (size_t i = 0; i < a1_size; ++i)
      if (a1[i] != a2[i])
        return false;

    return true;
  }

  template <typename Ty>
  requires concepts::printable<Ty>
  void serialize_arr_preview(
    Ty const *const arr,
    size_t const size,
    std::stringstream &ss)
  {
    ss << "sz=" << size;

    if (size == 0)
      return;

    ss << " __[__ ";

    size_t const max_len = std::min(size, ntest::internal::max_arr_preview_len());
    for (size_t i = 0; i < max_len; ++i)
    {
      ss << "<span style='" << internal::preview_style() << "' title='index " << i << "'>";
      if constexpr (std::is_integral_v<Ty>)
      {
        // some integrals like uint8_t are treated strangely by ostream insertion,
        // so casting must be done
        if constexpr (std::is_unsigned_v<Ty>)
          ss << static_cast<uintmax_t>(arr[i]);
        else
          ss << static_cast<intmax_t>(arr[i]);
      }
      else
      {
        ss << arr[i];
      }
      ss << "</span>, ";
    }

    if (size > max_len)
    {
      ss << " *... " << (size - max_len) << " more* ";
    }
    ss << "__]__";
  }

  std::string beautify_typeid_name(char const *name);

} // namespace internal

void init();

void assert_bool(
  bool expected,
  bool actual,
  std::source_location loc = std::source_location::current()
);

void assert_int8(
  int8_t expected,
  int8_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_uint8(
  uint8_t expected,
  uint8_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_int16(
  int16_t expected,
  int16_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_uint16(
  uint16_t expected,
  uint16_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_int32(
  int32_t expected,
  int32_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_uint32(
  uint32_t expected,
  uint32_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_int64(
  int64_t expected,
  int64_t actual,
  std::source_location loc = std::source_location::current()
);

void assert_uint64(
  uint64_t expected,
  uint64_t actual,
  std::source_location loc = std::source_location::current()
);

struct str_opts
{
  bool escape_special_chars;
};

str_opts default_str_opts();

void assert_cstr(
  char const *expected,
  char const *actual,
  str_opts const &options = default_str_opts(),
  std::source_location loc = std::source_location::current()
);

void assert_cstr(
  char const *expected,
  size_t expected_len,
  char const *actual,
  size_t actual_len,
  str_opts const &options = default_str_opts(),
  std::source_location loc = std::source_location::current()
);

void assert_stdstr(
  std::string const &expected,
  std::string const &actual,
  str_opts const &options = default_str_opts(),
  std::source_location loc = std::source_location::current()
);

template <typename Ty>
requires concepts::comparable_neq<Ty> && concepts::printable<Ty>
void assert_arr(
  Ty const *const expected,
  size_t const expected_size,
  Ty const *const actual,
  size_t const actual_size,
  std::source_location const loc = std::source_location::current())
{
  if (expected_size > 0 && expected == nullptr) {
    std::stringstream err{};
    err
      << "ntest::assert_arr - 'expected' array size was > 0 but data was nullptr, at "
      << loc.file_name() << ':' << loc.line() << "," << loc.column();
    throw std::runtime_error(err.str());
  }
  if (actual_size > 0 && actual == nullptr) {
    std::stringstream err{};
    err
      << "ntest::assert_arr - 'actual' array size was > 0 but data was nullptr, at "
      << loc.file_name() << ':' << loc.line() << "," << loc.column();
    throw std::runtime_error(err.str());
  }

  bool const passed = ntest::internal::arr_eq(
    expected, expected_size, actual, actual_size);

  std::stringstream serialized_vals{};
  serialized_vals
    << ntest::internal::beautify_typeid_name(typeid(Ty).name())
    << " [] | ";

  if (passed)
  {
    ntest::internal::serialize_arr_preview(expected, expected_size, serialized_vals);
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    std::string const
      expected_pathname = internal::generate_file_pathname(loc, "expected"),
      actual_pathname = internal::generate_file_pathname(loc, "actual");

    ntest::internal::write_arr_to_file(expected_pathname, expected, expected_size);
    ntest::internal::write_arr_to_file(actual_pathname, actual, actual_size);

    serialized_vals
      << '[' << expected_pathname << "](" << expected_pathname
      << ") | [" << actual_pathname << "](" << actual_pathname << ')';

    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

template <typename Ty>
requires concepts::comparable_neq<Ty> && concepts::printable<Ty>
void assert_stdvec(
  std::vector<Ty> const &expected,
  std::vector<Ty> const &actual,
  std::source_location const loc = std::source_location::current())
{
  bool const passed = ntest::internal::arr_eq(
    expected.data(), expected.size(), actual.data(), actual.size());

  std::stringstream serialized_vals{};
  serialized_vals
    << "std::vector\\<"
    << ntest::internal::beautify_typeid_name(typeid(Ty).name())
    << "\\> | ";

  if (passed)
  {
    ntest::internal::serialize_arr_preview(expected.data(), expected.size(), serialized_vals);
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    std::string const
      expected_pathname = internal::generate_file_pathname(loc, "expected"),
      actual_pathname = internal::generate_file_pathname(loc, "actual");

    ntest::internal::write_arr_to_file(expected_pathname, expected.data(), expected.size());
    ntest::internal::write_arr_to_file(actual_pathname, actual.data(), actual.size());

    serialized_vals
      << '[' << expected_pathname << "](" << expected_pathname
      << ") | [" << actual_pathname << "](" << actual_pathname << ')';

    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

template <typename Ty, size_t Size>
requires concepts::comparable_neq<Ty> && concepts::printable<Ty>
void assert_stdarr(
  std::array<Ty, Size> const &expected,
  std::array<Ty, Size> const &actual,
  std::source_location const loc = std::source_location::current())
{
  bool const passed = ntest::internal::arr_eq(
    expected.data(), expected.size(), actual.data(), actual.size());

  std::stringstream serialized_vals{};
  serialized_vals
    << "std::array\\<"
    << ntest::internal::beautify_typeid_name(typeid(Ty).name())
    << ", " << Size << "\\> | ";

  if (passed)
  {
    ntest::internal::serialize_arr_preview(expected.data(), expected.size(), serialized_vals);
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    std::string const
      expected_pathname = internal::generate_file_pathname(loc, "expected"),
      actual_pathname = internal::generate_file_pathname(loc, "actual");

    ntest::internal::write_arr_to_file(expected_pathname, expected.data(), expected.size());
    ntest::internal::write_arr_to_file(actual_pathname, actual.data(), actual.size());

    serialized_vals
      << '[' << expected_pathname << "](" << expected_pathname
      << ") | [" << actual_pathname << "](" << actual_pathname << ')';

    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

struct text_file_opts
{
  bool canonicalize_newlines;
};

text_file_opts default_text_file_opts();

void assert_text_file(
  char const *expected_pathname,
  char const *actual_pathname,
  text_file_opts const &options = default_text_file_opts(),
  std::source_location loc = std::source_location::current()
);

void assert_text_file(
  std::string const &expected_pathname,
  std::string const &actual_pathname,
  text_file_opts const &options = default_text_file_opts(),
  std::source_location loc = std::source_location::current()
);

void assert_text_file(
  std::filesystem::path const &expected,
  std::filesystem::path const &actual,
  text_file_opts const &options = default_text_file_opts(),
  std::source_location loc = std::source_location::current()
);

void assert_binary_file(
  char const *expected_pathname,
  char const *actual_pathname,
  std::source_location loc = std::source_location::current()
);

void assert_binary_file(
  std::string const &expected_pathname,
  std::string const &actual_pathname,
  std::source_location loc = std::source_location::current()
);

void assert_binary_file(
  std::filesystem::path const &expected,
  std::filesystem::path const &actual,
  std::source_location loc = std::source_location::current()
);

/*
  Asserts that a certain type of exception if thrown by `code_snippet`.
  If the correct exception type is thrown, returns the .what() string of the thrown exception.
  If the wrong exception type is thrown or no exception is thrown, an empty string is returned.
  `ExceptTy` must be derived from std::exception.
*/
template <typename ExceptTy>
requires concepts::derives_from_std_exception<ExceptTy>
[[maybe_unused]] std::string assert_throws(
  std::function<void (void)> const &code_snippet,
  std::source_location const loc = std::source_location::current())
{
  bool threw_correct_except = false;
  bool threw_incorrect_except = false;
  std::string what_str = "";

  try
  {
    code_snippet();
  }
  catch (ExceptTy const &except)
  {
    threw_correct_except = true;
    what_str = except.what();
  }
  catch (...)
  {
    threw_incorrect_except = true;
  }

  std::stringstream serialized_vals{};
  serialized_vals << ntest::internal::beautify_typeid_name(typeid(ExceptTy).name()) << " | ";

  if (threw_correct_except) // passed
  {
    serialized_vals << "to be thrown";
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else if (threw_incorrect_except) // failed
  {
    serialized_vals << "to be thrown | different exception type was thrown";
    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
  else // failed, didn't throw anything
  {
    serialized_vals << "to be thrown | nothing was thrown";
    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }

  return what_str;
}

void generate_report(char const *name);

size_t pass_count();

size_t fail_count();

} // namespace ntest

#endif // NLUKA_NTEST_HPP
