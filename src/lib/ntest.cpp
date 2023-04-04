#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>

#ifdef __GNUC__
# include <cxxabi.h>
#endif

#include "ntest.hpp"

static std::pair<char, char> constexpr s_special_chars[] {
  { '\a', 'a' },
  { '\b', 'b' },
  { '\f', 'f' },
  { '\n', 'n' },
  { '\r', 'r' },
  { '\t', 't' },
  { '\v', 'v' },
  { '\0', '0' },
  { '`', '`' }, // Due to markdown
};

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

// STATE:
static vector<assertion> s_failed_assertions{};
static vector<assertion> s_passed_assertions{};

// CONFIGURABLE SETTINGS:
static size_t s_max_str_preview_len = 20;
static size_t s_max_arr_preview_len = 10;

void ntest::config::set_max_str_preview_len(size_t const len)
{
  s_max_str_preview_len = len;
}

void ntest::config::set_max_arr_preview_len(size_t const len)
{
  s_max_arr_preview_len = len;
}

char const *ntest::internal::preview_style()
{
  return
    "border: 1px solid darkgray;"
    "font-family: monospace;"
    "padding: 2px;"
    "white-space: pre-wrap;"
  ;
}

size_t ntest::internal::max_str_preview_len()
{
  return s_max_str_preview_len;
}

size_t ntest::internal::max_arr_preview_len()
{
  return s_max_arr_preview_len;
}

void ntest::internal::register_failed_assertion(
  stringstream &&ss,
  source_location const &loc)
{
  s_failed_assertions.emplace_back(ss.str(), loc);
}

void ntest::internal::register_passed_assertion(
  stringstream &&ss,
  source_location const &loc)
{
  s_passed_assertions.emplace_back(ss.str(), loc);
}

string ntest::internal::make_stringified_file_path(
  source_location const &loc,
  char const *const extension)
{
  stringstream path{};

  path
    << fs::path(loc.file_name()).filename().generic_string()
    // Windows doesn't like : so use @ instead
    << '@' << loc.line() << ',' << loc.column()
    << '.' << extension;

  return path.str();
}

void ntest::internal::throw_if_file_not_open(
  fstream const &file,
  char const *const path)
{
  if (!file.is_open())
  {
    stringstream err{};
    err << "failed to open file \"" << path << '"';
    throw runtime_error(err.str());
  }
}

/*
  MSVC loves generating very ugly typeids, this cleans them up.
  g++ loves mangling names, this undoes that.
*/
string ntest::internal::beautify_typeid_name(char const *const name)
{
  using std::regex;

  string pretty_name;
#ifdef __GNUC__
  int status;
  char const *const unmangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  assert(status == 0);
  pretty_name = unmangled;
  free((void *)unmangled);
#else
  pretty_name = name;
#endif

  static std::pair<regex, char const *> const operations[] {
    { regex(" +([<>])"), "$1" },
    { regex(",([^ ])"), ", $1" },
    { regex(" {2,}"), " " },
  };

  for (auto const &[regex, replacement] : operations)
    pretty_name = std::regex_replace(pretty_name, regex, replacement);

  return pretty_name;
}

template <typename Ty>
requires std::integral<Ty>
void assert_integral(
  Ty const expected,
  Ty const actual,
  source_location const &loc)
{
  bool const passed = actual == expected;

  stringstream serialized_vals{};
  serialized_vals
    << ntest::internal::beautify_typeid_name(typeid(expected).name())
    << " | " << std::to_string(expected);

  if (passed)
  {
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    serialized_vals << " | " << std::to_string(actual);
    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

static
char const *bool_to_string(bool const b)
{
  if (b == true) {
    return "true";
  } else {
    return "false";
  }
}

void ntest::assert_bool(
  bool const expected,
  bool const actual,
  std::source_location const loc)
{
  bool const passed = actual == expected;

  stringstream serialized_vals{};
  serialized_vals << "bool | " << bool_to_string(expected);

  if (passed)
  {
    ntest::internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    serialized_vals << " | " << bool_to_string(actual);
    ntest::internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

void ntest::assert_int8(
  int8_t const expected,
  int8_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint8(
  uint8_t const expected,
  uint8_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int16(
  int16_t const expected,
  int16_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint16(
  uint16_t const expected,
  uint16_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int32(
  int32_t const expected,
  int32_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint32(
  uint32_t const expected,
  uint32_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_int64(
  int64_t const expected,
  int64_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

void ntest::assert_uint64(
  uint64_t const expected,
  uint64_t const actual,
  source_location const loc)
{
  assert_integral(expected, actual, loc);
}

std::string ntest::internal::escape(std::string const &input)
{
  std::string out{};
  out.reserve(input.length());

  for (size_t i = 0; i < input.length(); ++i)
  {
    char const ch = input[i];
    bool is_special = false;

    for (auto const &pair : s_special_chars)
    {
      if (ch == pair.first)
      {
        is_special = true;
        out += '\\';
        out += pair.second;
        break;
      }
    }

    if (!is_special)
      out += ch;
  }

  return std::move(out);
}

static
void print_escaped_string_to_ostream(
  char const *const str,
  size_t const len,
  std::ostream &os)
{
  for (size_t i = 0; i < len; ++i)
  {
    char const ch = str[i];
    bool is_special = false;

    for (auto const &pair : s_special_chars)
    {
      if (ch == pair.first)
      {
        is_special = true;
        os << '\\' << pair.second;
        break;
      }
    }

    if (!is_special)
      os << ch;
  }
}

static
void serialize_str_preview(
  char const *const str,
  size_t const len,
  stringstream &ss)
{
  ss << "len=" << len;

  if (len == 0)
    return;

  size_t const max_len = ntest::internal::max_str_preview_len();

  ss << " <span style='" << ntest::internal::preview_style() << "'>";
  {
    std::string_view const preview_content(str, std::min(len, max_len));
    print_escaped_string_to_ostream(preview_content.data(), preview_content.size(), ss);
  }
  ss << "</span>";

  if (len > max_len)
  {
    size_t const num_hidden_chars = len - max_len;
    ss << " *... " << num_hidden_chars << " more*";
  }
}

void ntest::assert_cstr(
  char const *const expected,
  char const *const actual,
  ntest::str_opts const &options,
  source_location const loc)
{
  size_t const expected_len = strlen(expected);
  size_t const actual_len = strlen(actual);
  ntest::assert_cstr(expected, expected_len, actual, actual_len, options, loc);
}

ntest::str_opts ntest::default_str_opts()
{
  str_opts const s_options = { true };
  return s_options;
}

void ntest::assert_cstr(
  char const *const expected,
  size_t const expected_len,
  char const *const actual,
  size_t const actual_len,
  ntest::str_opts const &options,
  source_location const loc)
{
  bool const passed =
    (expected_len == actual_len) &&
    (strcmp(expected, actual) == 0)
  ;

  stringstream serialized_vals{};
  serialized_vals << "char* | ";

  if (passed)
  {
    serialize_str_preview(expected, expected_len, serialized_vals);
    internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    string const
      expected_path = internal::make_stringified_file_path(loc, "expected"),
      actual_path = internal::make_stringified_file_path(loc, "actual");

    auto const write_file = [&options](
      string const &path, char const *str, size_t const len)
    {
      fstream file(path, std::ios::out);
      internal::throw_if_file_not_open(file, path.c_str());

      if (options.escape_special_chars)
        print_escaped_string_to_ostream(str, len, file);
      else
        file << str;
    };

    write_file(expected_path, expected, expected_len);
    write_file(actual_path, actual, actual_len);

    serialized_vals
      << '[' << expected_path << "](" << expected_path
      << ") | [" << actual_path << "](" << actual_path << ')';

    internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

void ntest::assert_stdstr(
  string const &expected,
  string const &actual,
  str_opts const &options,
  source_location const loc)
{
  ntest::assert_cstr(expected.c_str(), expected.size(),
    actual.c_str(), actual.size(), options, loc);
}

static
string extract_text_file_contents(
  string const &path,
  ntest::text_file_opts const &options)
{
  fstream file(path, std::ios::in);
  ntest::internal::throw_if_file_not_open(file, path.c_str());

  auto const file_size = fs::file_size(path);
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
vector<uint8_t> extract_binary_file_contents(string const &path)
{
  fstream file(path, std::ios::in | std::ios::binary);
  ntest::internal::throw_if_file_not_open(file, path.c_str());

  auto const file_size = fs::file_size(path);

  std::vector<uint8_t> vec(file_size);
  file.read(reinterpret_cast<char *>(vec.data()), file_size);
  assert(file.gcount() > 0 && static_cast<uintmax_t>(file.gcount()) == file_size);

  return vec;
}

ntest::text_file_opts ntest::default_text_file_opts()
{
  static text_file_opts const s_options = { true };
  return s_options;
}

void ntest::assert_text_file(
  char const *const expected_path,
  char const *const actual_path,
  text_file_opts const &options,
  source_location const loc)
{
  assert_text_file(
    fs::path(expected_path), fs::path(actual_path), options, loc);
}

void ntest::assert_text_file(
  string const &expected_path,
  string const &actual_path,
  text_file_opts const &options,
  source_location const loc)
{
  assert_text_file(
    fs::path(expected_path), fs::path(actual_path), options, loc);
}

void ntest::assert_text_file(
  fs::path const &expected_path,
  fs::path const &actual_path,
  text_file_opts const &options,
  source_location const loc)
{
  bool expected_exists, actual_exists;
  {
    std::error_code ec{};
    expected_exists = fs::is_regular_file(expected_path, ec);
    actual_exists = fs::is_regular_file(actual_path, ec);
  }

  string const
    expected_path_generic = expected_path.generic_string(),
    actual_path_generic = actual_path.generic_string();

  string const
    expected = expected_exists
      ? extract_text_file_contents(expected_path_generic, options)
      : "",
    actual = actual_exists
      ? extract_text_file_contents(actual_path_generic, options)
      : "";

  bool const passed = expected_exists && actual_exists && expected == actual;

  stringstream serialized_vals{};

  serialized_vals << "text file | ";
  if (!expected_exists)
  {
    serialized_vals << "file not found";
  }
  else
  {
    serialized_vals
      << '[' << expected_path_generic << "]("
      << expected_path_generic << ')';
  }

  if (passed)
  {
    internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    serialized_vals << " | ";
    if (!actual_exists)
      serialized_vals << "file not found";
    else
    {
      serialized_vals
        << '[' << actual_path_generic << "]("
        << actual_path_generic << ')';
    }
    internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

void ntest::assert_binary_file(
  char const *const expected_path,
  char const *const actual_path,
  source_location const loc)
{
  assert_binary_file(
    fs::path(expected_path), fs::path(actual_path), loc);
}

void ntest::assert_binary_file(
  string const &expected_path,
  string const &actual_path,
  source_location const loc)
{
  assert_binary_file(
    fs::path(expected_path), fs::path(actual_path), loc);
}

void ntest::assert_binary_file(
  fs::path const &expected_path,
  fs::path const &actual_path,
  source_location const loc)
{
  bool expected_exists, actual_exists;
  {
    std::error_code ec{};
    expected_exists = fs::is_regular_file(expected_path, ec);
    actual_exists = fs::is_regular_file(actual_path, ec);
  }

  string const
    expected_path_generic = expected_path.generic_string(),
    actual_path_generic = actual_path.generic_string();

  vector<uint8_t> const expected = [
    expected_exists, &expected_path_generic]()
  {
    if (expected_exists)
      return extract_binary_file_contents(expected_path_generic);
    else
      return vector<uint8_t>();
  }();

  vector<uint8_t> const actual = [actual_exists, &actual_path_generic]()
  {
    if (actual_exists)
      return extract_binary_file_contents(actual_path_generic);
    else
      return vector<uint8_t>();
  }();

  bool const passed = expected_exists && actual_exists &&
    internal::arr_eq(expected.data(), expected.size(),
      actual.data(), actual.size());

  stringstream serialized_vals{};

  serialized_vals << "binary file | ";
  if (!expected_exists)
    serialized_vals << "file not found";
  else
  {
    serialized_vals
      << '[' << expected_path_generic << "]("
      << expected_path_generic << ')';
  }

  if (passed)
  {
    internal::register_passed_assertion(std::move(serialized_vals), loc);
  }
  else // failed
  {
    serialized_vals << " | ";
    if (!actual_exists)
      serialized_vals << "file not found";
    else
    {
      serialized_vals
        << '[' << actual_path_generic << "]("
        << actual_path_generic << ')';
    }
    internal::register_failed_assertion(std::move(serialized_vals), loc);
  }
}

static
std::string path_minus_dir_overlap(fs::path subject_abs, fs::path directory_abs)
{
  assert(fs::exists(subject_abs));
  assert(fs::is_regular_file(subject_abs));
  assert(subject_abs.is_absolute());

  assert(fs::is_directory(directory_abs));
  assert(directory_abs.is_absolute());

  string subj_str = subject_abs.string();
  string dir_str = directory_abs.string();

  assert(!subj_str.empty());
  assert(!dir_str.empty());

  if (!dir_str.ends_with(fs::path::preferred_separator))
    dir_str.push_back(fs::path::preferred_separator);

  size_t i;
  for (i = 0; i < dir_str.size() && subj_str[i] == dir_str[i]; ++i);

  char const *const result = subj_str.c_str() + i;

  return std::string(result);
}

ntest::report_result ntest::generate_report(char const *const name)
{
  size_t const
    total_failed = s_failed_assertions.size(),
    total_passed = s_passed_assertions.size();

  string report_path = "./";
  report_path.append(name);
  report_path.append(".md");

  std::ofstream ofs(report_path, std::ios::out);

  {
    time_t const raw_time = time(nullptr);
    char const *const time_cstr = ctime(&raw_time);

    ofs
      << "# " << name << "\n\n"
      << time_cstr << "\n" // only 1 \n because ctime result has 1 already
      << total_failed << " failed\n\n"
      << total_passed << " passed\n\n"
    ;
  }

  auto const print_table_row = [&ofs](assertion const &assertion, bool const passed)
  {
    auto const &[serialized_vals, loc] = assertion;
    ofs
      // Outcome
      << "| " << (passed ? "✅" : "❌") << ' '
      // Type, Expected, [Actual]
      << "| " << serialized_vals << ' '
      // Location
      << "| " << loc.function_name() << ':' << loc.line() << ',' << loc.column() << ' '
      // Source File
      << "| " << path_minus_dir_overlap(fs::absolute(loc.file_name()), fs::absolute(fs::current_path())) << " |\n"
    ;
  };

  if (total_failed > 0)
  {
    ofs
      << "| | Type | Expected | Actual | Location (fn:ln,col) | Source File |\n"
      << "| - | - | - | - | - | - |\n"
    ;

    for (auto const &assertion : s_failed_assertions)
      print_table_row(assertion, false);

    ofs << '\n';
  }

  if (total_passed > 0)
  {
    ofs
      << "| | Type | Expected | Location (fn:ln,col) | Source File |\n"
      << "| - | - | - | - | - |\n"
    ;

    for (auto const &assertion : s_passed_assertions)
      print_table_row(assertion, true);

    ofs << '\n';
  }

  // reset state to allow user to generate multiple independent reports
  s_failed_assertions.clear();
  s_passed_assertions.clear();

  return { total_passed, total_failed };
}

ntest::init_result ntest::init(bool const remove_residual_files)
{
  auto const current_path = fs::current_path();
  size_t num_files_removed = 0;
  size_t num_files_failed_to_remove = 0;

  if (remove_residual_files)
  {
    // remove any residual .expected and .actual files
    for (
      auto const &entry :
      fs::directory_iterator(
        current_path,
        fs::directory_options::skip_permission_denied
      )
    )
    {
      if (!entry.is_regular_file())
        continue;

      auto const path = entry.path();
      if (!path.has_extension())
        continue;

      auto const extension = path.extension();
      if (extension == ".expected" || extension == ".actual")
      {
        try
        {
          fs::remove(path);
          ++num_files_removed;
        }
        catch (std::runtime_error const &except)
        {
          std::stringstream err{};
          err << "failed to remove " << path << ", " << except.what();
          std::cerr << err.str() << '\n';
          ++num_files_failed_to_remove;
        }
      }
    }
  }

  return { num_files_removed, num_files_failed_to_remove };
}

// Returns the number of passed assertions since the last time `ntest::generate_report` was called.
size_t ntest::pass_count()
{
  return s_passed_assertions.size();
}

// Returns the number of failed assertions since the last time `ntest::generate_report` was called.
size_t ntest::fail_count()
{
  return s_failed_assertions.size();
}
