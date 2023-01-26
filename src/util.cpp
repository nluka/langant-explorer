#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "exit.hpp"
#include "types.hpp"
#include "util.hpp"

using namespace std;

fstream util::open_file(char const *const pathname, int const flags) {
  bool const is_for_reading = (flags & 1) == 1;
  if (is_for_reading) {
    if (!filesystem::exists(pathname)) {
      cerr << "fatal: file `" << pathname << "` not found\n";
      EXIT(exit_code::FILE_NOT_FOUND);
    }
  }

  fstream file(pathname, static_cast<ios_base::openmode>(flags));

  if (!file.is_open()) {
    cerr << "fatal: unable to open file `" << pathname << "`\n";
    EXIT(exit_code::FILE_OPEN_FAILED);
  }

  if (!file.good()) {
    cerr << "fatal: bad file `" << pathname << "`\n";
    EXIT(exit_code::BAD_FILE);
  }

  return file;
}

string util::extract_txt_file_contents(char const *const pathname) {
  fstream file = util::open_file(pathname, ios::in);
  auto const fileSize = filesystem::file_size(pathname);

  string content{};
  content.reserve(fileSize);

  getline(file, content, '\0');

  // remove any \r characters
  content.erase(
    remove(content.begin(), content.end(), '\r'),
    content.end()
  );

  return content;
}

string util::format_file_size(usize const size) {
  char out[21];
  format_file_size(size, out, sizeof(out));
  return string(out);
}

void util::format_file_size(
  usize const size_,
  char *const out,
  usize const out_size
) {
  char const *units[] = { "B", "KB", "MB", "GB", "TB" };
  usize constexpr biggest_unit_idx = util::lengthof(units) - 1;
  usize unit_idx = 0;

  double size = static_cast<double>(size_);

  while (size >= 1024 && unit_idx < biggest_unit_idx) {
    size /= 1024;
    ++unit_idx;
  }

  char const *const fmt =
    unit_idx == 0
    // no digits after decimal point for bytes
    // because showing a fraction of a byte doesn't make sense
    ? "%.0lf %s"
    // 2 digits after decimal points for denominations
    // greater than bytes
    : "%.2lf %s";

  snprintf(out, out_size, fmt, size, units[unit_idx]);
}
