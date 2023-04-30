#include <cstdarg>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "json.hpp"
#include "term.hpp"
#include "primitives.hpp"
#include "util.hpp"

using namespace std;
using json_t = nlohmann::json;

util::time_span::time_span(u64 const seconds_elapsed) noexcept
{
  u64 const SECONDS_PER_DAY = 86400, SECONDS_PER_HOUR = 3600, SECONDS_PER_MINUTE = 60;

  this->days = 0;
  this->hours = 0;
  this->minutes = 0;
  this->seconds = seconds_elapsed;

  while (this->seconds >= SECONDS_PER_DAY) {
    ++this->days;
    this->seconds -= SECONDS_PER_DAY;
  }

  while (this->seconds >= SECONDS_PER_HOUR) {
    ++this->hours;
    this->seconds -= SECONDS_PER_HOUR;
  }

  while (this->seconds >= SECONDS_PER_MINUTE) {
    ++this->minutes;
    this->seconds -= SECONDS_PER_MINUTE;
  }
}

class static_streambuf : public std::streambuf {
public:
  static_streambuf(char *const base, u64 const size) {
    setp(base, base + size);
  }

protected:
  virtual int_type overflow(int_type const ch) override {
    // Buffer is full, discard the character or handle overflow in another way
    return traits_type::not_eof(ch);
  }
};

char *util::time_span::stringify(char *const out, u64 const out_len) const
{
  static_streambuf buf(out, out_len);
  std::ostream os(&buf);

  if (this->days > 0) {
    os << this->days << "d ";
  }
  if (this->hours > 0) {
    os << this->hours << "h ";
  }
  if (this->minutes > 0) {
    os << this->minutes << "m ";
  }
  os << this->seconds << 's';
  os << '\0';

  return out;
}

string util::time_span::to_string() const
{
  std::stringstream ss;

  if (this->days > 0) {
    ss << this->days << "d ";
  }
  if (this->hours > 0) {
    ss << this->hours << "h ";
  }
  if (this->minutes > 0) {
    ss << this->minutes << "m ";
  }
  ss << this->seconds << 's';

  return ss.str();
}

util::time_point_t util::current_time()
{
  return std::chrono::high_resolution_clock::now();
}

u64 util::nanos_between(
  time_point_t const start,
  time_point_t const end)
{
  auto const nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
  assert(nanos.count() >= 0);
  return static_cast<u64>(nanos.count());
}

string util::make_str(char const *const fmt, ...)
{
  i32 const buf_len = 1024;
  static char buffer[buf_len];

  va_list args;
  va_start(args, fmt);
  [[maybe_unused]] i32 const cnt = vsnprintf(buffer, sizeof(buffer), fmt, args);
  assert(cnt > 0);
  assert(cnt < buf_len);
  va_end(args);

  return std::string(buffer);
}

std::fstream util::open_file(
  std::string const &path,
  std::ios_base::openmode const flags)
{
  return util::open_file(path.c_str(), flags);
}

std::fstream util::open_file(
  char const *const path,
  std::ios_base::openmode const flags)
{
  b8 const is_for_reading = (flags & std::ios::in) == std::ios::in;
  if (is_for_reading && !std::filesystem::exists(path))
    throw "not found";

  std::fstream file(path, static_cast<std::ios_base::openmode>(flags));

  if (!file)
    throw "exists, but unable to open";

  return file;
}

b8 util::file_is_openable(std::string const &path)
{
  std::ofstream file(path.c_str(), std::ios::app);
  return file ? true : false;
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

  for (u64 i = 0; i < json.size(); ++i) {
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
  [[maybe_unused]] i32 const retval = vprintf(fmt, args);
  va_end(args);

  term::color::set(fore::DEFAULT | back::BLACK);

  putc('\n', stdout);

  std::exit(1);
}

int util::print_err(char const *fmt, ...)
{
  using namespace term::color;

  term::color::set(fore::RED | back::BLACK);

  va_list args;
  va_start(args, fmt);
  i32 const retval = vprintf(fmt, args);
  va_end(args);

  term::color::set(fore::DEFAULT | back::BLACK);

  putc('\n', stdout);

  return retval;
}

std::string util::stringify_errors(util::errors_t const &errors)
{
  u64 length = 0;
  for (auto const &err : errors)
    length += err.length() + 2;

  std::string out;
  out.reserve(length);
  for (auto const &err : errors)
    out += err;
    out += "; ";

  out.pop_back(); // remove trailing space

  return out;
}

b8 util::get_user_choice(std::string const &prompt)
{
  {
    using namespace term::color;
    printf(fore::YELLOW | back::BLACK, "%s [y/n] ", prompt.c_str());
  }

  std::string input;
  std::cin >> input;

  char const first_input_chr_lowercase = static_cast<char>(tolower(input.front()));
  return first_input_chr_lowercase == 'y';
}
