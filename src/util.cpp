#include <cstdarg>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "lib/json.hpp"

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

fstream util::open_file(char const *const pathname, int const flags)
{
  b8 const is_for_reading = (flags & 1) == 1;
  if (is_for_reading && !filesystem::exists(pathname)) {
    throw runtime_error(make_str("file `%s` not found", pathname));
  }

  fstream file(pathname, static_cast<ios_base::openmode>(flags));

  if (!file.is_open()) {
    throw runtime_error(make_str("unable to open file `%s`", pathname));
  }

  if (!file.good()) {
    throw runtime_error(make_str("bad file `%s`", pathname));
  }

  return file;
}

string util::extract_txt_file_contents(char const *const pathname, bool const normalize_newlines)
{
  fstream file = util::open_file(pathname, ios::in);
  auto const fileSize = filesystem::file_size(pathname);

  string content{};
  content.reserve(fileSize);

  getline(file, content, '\0');

  if (normalize_newlines) {
    // remove any \r characters
    content.erase(remove(content.begin(), content.end(), '\r'), content.end());
  }

  return content;
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
