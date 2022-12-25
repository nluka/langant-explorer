#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "ntest.hpp"

template <typename Ty, size_t Length>
consteval
size_t lengthof(Ty (&)[Length])
{
  return Length;
}

namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::source_location;
using std::stringstream;
using std::runtime_error;
using std::fstream;
using ntest::internal::assertion;

static vector<assertion> s_failed_assertions{};
static vector<assertion> s_passed_assertions{};
static string s_output_path = ".";
static size_t s_max_str_preview_len = 20;
static size_t s_max_arr_preview_len = 10;
static std::pair<char, char> constexpr s_special_chars[] {
  { '\a', 'a' },
  { '\b', 'b' },
  { '\f', 'f' },
  { '\n', 'n' },
  { '\r', 'r' },
  { '\t', 't' },
  { '\v', 'v' },
  { '\0', '0' },
};

void ntest::config::set_output_path(char const *const path)
{
  s_output_path = path;
}

void ntest::config::set_max_str_preview_len(size_t const len)
{
  s_max_str_preview_len = len;
}

void ntest::config::set_max_arr_preview_len(size_t const len)
{
  s_max_arr_preview_len = len;
}

size_t ntest::internal::max_str_preview_len()
{
  return s_max_str_preview_len;
}

size_t ntest::internal::max_arr_preview_len()
{
  return s_max_arr_preview_len;
}

void ntest::internal::emplace_failed_assertion(
  stringstream const &ss, source_location const &loc)
{
  s_failed_assertions.emplace_back(std::move(ss.str()), loc);
}

void ntest::internal::emplace_passed_assertion(
  stringstream const &ss, source_location const &loc)
{
  s_passed_assertions.emplace_back(std::move(ss.str()), loc);
}

string ntest::internal::generate_file_pathname(
  source_location const &loc, char const *const extension)
{
  stringstream pathname{};

  pathname
    << fs::path(loc.file_name()).filename().generic_string() << '@'
    << loc.function_name() << '(' << loc.line() << ',' << loc.column() << ')'
    << '.' << extension;

  return pathname.str();
}

void ntest::internal::throw_if_file_not_open(
  fstream const &file, char const *const pathname)
{
  if (!file.is_open())
  {
    stringstream err{};
    err << "failed to open file \"" << pathname << '"';
    throw runtime_error(err.str());
  }
}

string ntest::internal::beautify_typeid_name(char const *const name)
{
  using std::regex;

  static std::pair<regex, char const *> const operations[] {
    { regex(" +([<>])"), "$1" },
    { regex(",([^ ])"), ", $1" },
    { regex(" {2,}"), " " },
  };

  string pretty_name = name;
  for (auto const &[regex, replacement] : operations)
    pretty_name = std::regex_replace(pretty_name, regex, replacement);

  return pretty_name;
}

template <typename Ty>
requires std::integral<Ty>
void assert_integral(
  Ty const expected, Ty const actual,
  source_location const &loc)
{
  bool const passed = actual == expected;

  stringstream serialized_vals{};
  serialized_vals
    << ntest::internal::beautify_typeid_name(typeid(expected).name())
    << " | " << std::to_string(expected);

  if (passed)
    ntest::internal::emplace_passed_assertion(serialized_vals, loc);
  else // failed
  {
    serialized_vals << " | " << std::to_string(actual);
    ntest::internal::emplace_failed_assertion(serialized_vals, loc);
  }
}

void ntest::assert_int8(
  int8_t const expected, int8_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint8(
  uint8_t const expected, uint8_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int16(
  int16_t const expected, int16_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint16(
  uint16_t const expected, uint16_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int32(
  int32_t const expected, int32_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint32(
  uint32_t const expected, uint32_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int64(
  int64_t const expected, int64_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint64(
  uint64_t const expected, uint64_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

static
void serialize_str_preview(
  char const *const str, size_t const len, stringstream &ss)
{
  ss << "len=" << len;

  if (len == 0)
    return;

  size_t const max_len = ntest::internal::max_str_preview_len();

  ss << " \"`";
  {
    std::string_view const content(str, std::min(len, max_len));

    for (size_t i = 0; i < content.size(); ++i)
    {
      char const ch = content[i];
      bool is_special = false;

      for (auto const &pair : s_special_chars)
        if (ch == pair.first)
        {
          is_special = true;
          ss << '\\' << pair.second;
          break;
        }

      if (!is_special)
        ss << ch;
    }
  }
  ss << '`';

  if (len <= max_len)
    ss << '"';
  else
  {
    size_t const num_hidden_chars = len - max_len;
    ss << "*... " << num_hidden_chars << " more*";
  }
}

void ntest::assert_cstr(
  char const *const expected, char const *const actual,
  ntest::str_opts const &options,
  source_location const loc)
{
  size_t const expected_len = strlen(expected);
  size_t const actual_len = strlen(actual);
  ntest::assert_cstr(expected, expected_len, actual, actual_len, options, loc);
}

ntest::str_opts ntest::default_str_opts()
{
  static str_opts const s_options = { true };
  return s_options;
}

void ntest::assert_cstr(
  char const *const expected, size_t const expected_len,
  char const *const actual, size_t const actual_len,
  ntest::str_opts const &options,
  source_location const loc)
{
  bool const passed = strcmp(expected, actual) == 0;

  stringstream serialized_vals{};
  serialized_vals << "char* | ";

  if (passed)
  {
    serialize_str_preview(expected, expected_len, serialized_vals);
    internal::emplace_passed_assertion(serialized_vals, loc);
  }
  else // failed
  {
    string const
      expected_pathname = internal::generate_file_pathname(loc, "expected"),
      actual_pathname = internal::generate_file_pathname(loc, "actual");

    auto const write_file = [&options](
      string const &pathname, char const *str, size_t const len)
    {
      fstream file(pathname, std::ios::out);
      internal::throw_if_file_not_open(file, pathname.c_str());

      if (!options.escape_special_chars)
      {
        file << str;
        return;
      }

      for (size_t i = 0; i < len; ++i)
      {
        char const ch = str[i];
        bool is_special = false;

        for (auto const &pair : s_special_chars)
          if (ch == pair.first)
          {
            is_special = true;
            file << '\\' << pair.second;
            break;
          }

        if (!is_special)
          file << ch;
      }
    };

    write_file(expected_pathname, expected, expected_len);
    write_file(actual_pathname, actual, actual_len);

    serialized_vals
      << '[' << expected_pathname << "](" << expected_pathname
      << ") | [" << actual_pathname << "](" << actual_pathname << ')';

    internal::emplace_failed_assertion(serialized_vals, loc);
  }
}

void ntest::assert_stdstr(
  string const &expected, string const &actual,
  str_opts const &options, source_location loc)
{
  ntest::assert_cstr(expected.c_str(), expected.size(),
    actual.c_str(), actual.size(), options, loc);
}

// static
// string normalize_path(string path) {
//   // TODO: look into using realpath as a replacement for this function

//   for (size_t i = 0; i < path.length(); ++i)
//   {
//     if (strchr("\\/", path[i]))
//       path[i] = '/';
//     else
//       path[i] = std::tolower(path[i]);
//   }

//   static std::regex const adjacent_separators("/{2,}");

//   char const preferred_separator[2] { fs::path::preferred_separator, '\0' };

//   // replace occurences of 2+ adjacent separators with just 1 separator
//   string const good_separators = std::regex_replace(
//     path, adjacent_separators, preferred_separator);

//   return good_separators;
// }

// static
// char const *get_unique_path_piece(
//   char const *const base,
//   char const *const subject
// ) {
//   size_t const base_len = strlen(subject);
//   size_t const subject_len = strlen(subject);

//   // find pos of first differing char:
//   size_t i = 0;
//   while (
//     i < base_len &&
//     i < subject_len &&
//     base[i] == subject[i]
//   ) ++i;

//   return subject + i;
// }

static
string extract_text_file_contents(
  string const &pathname, ntest::text_file_opts const &options)
{
  fstream file(pathname, std::ios::in);
  ntest::internal::throw_if_file_not_open(file, pathname.c_str());

  auto const file_size = fs::file_size(pathname);
  string contents(file_size, '\0');
  file.read(contents.data(), file_size);

  if (options.canonicalize_newlines)
  {
    static std::regex const carriage_returns("\r");
    contents = std::regex_replace(contents, carriage_returns, "");
  }

  return contents;
}

static
vector<uint8_t> extract_binary_file_contents(string const &pathname)
{
  fstream file(pathname, std::ios::in | std::ios::binary);
  ntest::internal::throw_if_file_not_open(file, pathname.c_str());

  auto const file_size = fs::file_size(pathname);

  std::vector<uint8_t> vec(file_size);
  file.read(reinterpret_cast<char *>(vec.data()), file_size);

  return vec;
}

ntest::text_file_opts ntest::default_text_file_opts()
{
  static text_file_opts const s_options = { true };
  return s_options;
}

void ntest::assert_text_file(
  char const *const expected_pathname, char const *const actual_pathname,
  text_file_opts const &options, source_location const loc)
{
  assert_text_file(
    fs::path(expected_pathname), fs::path(actual_pathname), options, loc);
}

void ntest::assert_text_file(
  string const &expected_pathname, string const &actual_pathname,
  text_file_opts const &options, source_location const loc)
{
  assert_text_file(
    fs::path(expected_pathname), fs::path(actual_pathname), options, loc);
}

void ntest::assert_text_file(
  fs::path const &expected_pathname, fs::path const &actual_pathname,
  text_file_opts const &options, source_location const loc)
{
  bool expected_exists, actual_exists;
  {
    std::error_code ec{};
    expected_exists = fs::is_regular_file(expected_pathname, ec);
    actual_exists = fs::is_regular_file(actual_pathname, ec);
  }

  string const
    expected_pathname_generic = expected_pathname.generic_string(),
    actual_pathname_generic = actual_pathname.generic_string();

  string const
    expected = expected_exists
      ? extract_text_file_contents(expected_pathname_generic, options)
      : "",
    actual = actual_exists
      ? extract_text_file_contents(actual_pathname_generic, options)
      : "";

  bool const passed = expected_exists && actual_exists && expected == actual;

  stringstream serialized_vals{};

  serialized_vals << "text file | ";
  if (!expected_exists)
    serialized_vals << "<span style='color:red;'>not found</span>";
  else
  {
    serialized_vals
      << '[' << expected_pathname_generic << "]("
      << expected_pathname_generic << ')';
  }

  if (passed)
    internal::emplace_passed_assertion(serialized_vals, loc);
  else // failed
  {
    serialized_vals << " | ";
    if (!actual_exists)
      serialized_vals << "<span style='color:red;'>not found</span>";
    else
    {
      serialized_vals
        << '[' << actual_pathname_generic << "]("
        << actual_pathname_generic << ')';
    }
    internal::emplace_failed_assertion(serialized_vals, loc);
  }
}

void ntest::assert_binary_file(
  char const *const expected_pathname, char const *const actual_pathname,
  source_location const loc)
{
  assert_binary_file(
    fs::path(expected_pathname), fs::path(actual_pathname), loc);
}

void ntest::assert_binary_file(
  string const &expected_pathname, string const &actual_pathname,
  source_location const loc)
{
  assert_binary_file(
    fs::path(expected_pathname), fs::path(actual_pathname), loc);
}

void ntest::assert_binary_file(
  fs::path const &expected_pathname, fs::path const &actual_pathname,
  source_location const loc)
{
  bool expected_exists, actual_exists;
  {
    std::error_code ec{};
    expected_exists = fs::is_regular_file(expected_pathname, ec);
    actual_exists = fs::is_regular_file(actual_pathname, ec);
  }

  string const
    expected_pathname_generic = expected_pathname.generic_string(),
    actual_pathname_generic = actual_pathname.generic_string();

  vector<uint8_t> const expected = [
    expected_exists, &expected_pathname_generic]()
  {
    if (expected_exists)
      return extract_binary_file_contents(expected_pathname_generic);
    else
      return vector<uint8_t>();
  }();

  vector<uint8_t> const actual = [
    actual_exists, &actual_pathname_generic]()
  {
    if (actual_exists)
      return extract_binary_file_contents(actual_pathname_generic);
    else
      return vector<uint8_t>();
  }();

  bool const passed = expected_exists && actual_exists &&
    internal::arr_eq(expected.data(), expected.size(),
      actual.data(), actual.size());

  stringstream serialized_vals{};

  serialized_vals << "binary file | ";
  if (!expected_exists)
    serialized_vals << "<span style='color:red;'>not found</span>";
  else
  {
    serialized_vals
      << '[' << expected_pathname_generic << "]("
      << expected_pathname_generic << ')';
  }

  if (passed)
    internal::emplace_passed_assertion(serialized_vals, loc);
  else // failed
  {
    serialized_vals << " | ";
    if (!actual_exists)
      serialized_vals << "<span style='color:red;'>not found</span>";
    else
    {
      serialized_vals
        << '[' << actual_pathname_generic << "]("
        << actual_pathname_generic << ')';
    }
    internal::emplace_failed_assertion(serialized_vals, loc);
  }
}

void ntest::generate_report(char const *const name)
{
  size_t const
    total_failed = s_failed_assertions.size(),
    total_passed = s_passed_assertions.size();

  string pathname = s_output_path;
  if (!pathname.ends_with("/") && !pathname.ends_with("\\"))
    pathname.push_back('/');
  pathname.append(name);
  pathname.append(".md");

  std::ofstream ofs(pathname, std::ios::out);

  printf("generating report \"%s\"... ", pathname.c_str());

  ofs
    << "# " << name << "\n\n"
    << "|   |   |\n"
    << "| - | - |\n"
    << "| failed | " << total_failed << " |\n"
    << "| passed | " << total_passed << " |\n\n";

  auto const print_table_row = [&ofs](assertion const &assertion)
  {
    auto const &[serialized_vals, loc] = assertion;
    ofs
      << "| " << serialized_vals << " | "
      << loc.function_name() << ':' << loc.line() << ',' << loc.column()
      << " | [" << loc.file_name() << "](" << loc.file_name() << ") |\n";
  };

  if (total_failed > 0)
  {
    ofs
      << "## ❌ failed\n\n"
      << "| type | expected | actual | location (func:ln,col) | file |\n"
      << "| - | - | - | - | - |\n";

    for (auto const &assertion : s_failed_assertions)
      print_table_row(assertion);

    ofs << '\n';
  }

  if (total_passed > 0)
  {
    ofs
      << "## ✅ passed\n\n"
      << "| type | expected | location (func:ln,col) | file |\n"
      << "| - | - | - | - |\n";

    for (auto const &assertion : s_passed_assertions)
      print_table_row(assertion);

    ofs << '\n';
  }

  printf("done\n");

  // reset state to allow user to generate multiple independent reports
  s_failed_assertions.clear();
  s_passed_assertions.clear();
}
