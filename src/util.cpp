#include <cstdarg>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "lib/json.hpp"
#include "lib/term.hpp"

#include "primitives.hpp"
#include "util.hpp"

using namespace std;
using json_t = nlohmann::json;

util::time_point_t util::current_time()
{
  return std::chrono::high_resolution_clock::now();
}

std::chrono::nanoseconds util::nanos_between(time_point_t const a, time_point_t const b)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a);
}

string util::make_str(char const *const fmt, ...)
{
  usize const buf_len = 1024;
  char buffer[buf_len];

  va_list args;
  va_start(args, fmt);
  int const cnt = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // buffer[std::min(cnt, buf_len - 1)] = '\0';

  return std::string(buffer);
}

[[nodiscard]] std::fstream util::open_file(
  std::string const &path,
 std::ios_base::openmode const flags)
{
  return util::open_file(path.c_str(), flags);
}

[[nodiscard]] std::fstream util::open_file(
  char const *const path,
  std::ios_base::openmode const flags)
{
  b8 const is_for_reading = (flags & std::ios::in) == std::ios::in;
  if (is_for_reading && !std::filesystem::exists(path))
    throw "not found";

  std::fstream file(path, static_cast<std::ios_base::openmode>(flags));

  if (!file.is_open())
    throw "exists, but unable to open";
  if (!file.good())
    throw "bad state";

  return file;
}

/*
  Parses a string as a JSON array of uint64, throws:
  - json_t::basic_json::parse_error if `str` is not valid JSON of any kind
  - std::runtime_error if `str` is not a JSON array or some elements are not unsigned nums
*/
vector<u64> util::parse_json_array_u64(char const *str)
{
  json_t const json = json_t::parse(str);
  if (!json.is_array()) {
    throw std::runtime_error("not a JSON array");
  }

  std::vector<u64> save_points(json.size());

  for (usize i = 0; i < json.size(); ++i) {
    auto const &elem = json[i];
    if (!elem.is_number_unsigned()) {
      throw std::runtime_error(make_str("element at idx %zu is not unsigned number", i));
    }
    save_points[i] = elem.get<u64>();
  }

  return save_points;
}

void util::die(char const *fmt, ...)
{
  using namespace term::color;

  term::color::set(fore::RED | back::BLACK);

  printf("fatal: ");

  va_list args;
  va_start(args, fmt);
  [[maybe_unused]] int retval = vprintf(fmt, args);
  va_end(args);

  putc('\n', stdout);

  term::color::set(fore::DEFAULT | back::BLACK);

  std::exit(1);
}

int util::print_err(char const *fmt, ...)
{
  using namespace term::color;

  term::color::set(fore::RED | back::BLACK);

  va_list args;
  va_start(args, fmt);
  int retval = vprintf(fmt, args);
  va_end(args);

  putc('\n', stdout);

  term::color::set(fore::DEFAULT | back::BLACK);

  return retval;
}

bool util::user_wants_to_create_dir(std::string const &path)
{
  using namespace term::color;

  printf(
    fore::YELLOW | back::BLACK,
    "directory '%s' not found, would you like to create it? [y/n] ",
    path.c_str()
  );

  std::string input;

  std::cin >> input;

  char const first_lower = static_cast<char>(tolower(input.front()));
  return first_lower == 'y';
}
