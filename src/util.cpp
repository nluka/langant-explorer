#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "exit.hpp"
#include "util.hpp"

using namespace std;

bool util::in_range_incl_excl(int const val, int const min, int const max) {
  return val >= min && val < max;
}

fstream util::open_file(char const *const pathname, int const flags) {
  bool const forReading = (flags & 1) == 1;
  if (forReading) {
    if (!filesystem::exists(pathname)) {
      cerr << "fatal: file `" << pathname << "` not found\n";
      EXIT(ExitCode::FILE_NOT_FOUND);
    }
  }

  fstream file(pathname, static_cast<ios_base::openmode>(flags));

  if (!file.is_open()) {
    cerr << "fatal: unable to open file `" << pathname << "`\n";
    EXIT(ExitCode::FILE_OPEN_FAILED);
  }

  if (!file.good()) {
    cerr << "fatal: bad file `" << pathname << "`\n";
    EXIT(ExitCode::BAD_FILE);
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

string util::format_file_size(uintmax_t const size) {
  char out[21];
  format_file_size(size, out, sizeof(out));
  return string(out);
}

void util::format_file_size(
  uintmax_t const size_,
  char *const out,
  size_t const outSize
) {
  char const *units[] = { "B", "KB", "MB", "GB", "TB" };
  size_t constexpr largestUnitIdx = util::lengthof(units) - 1;
  size_t unitIdx = 0;

  double size = static_cast<double>(size_);

  while (size >= 1024 && unitIdx < largestUnitIdx) {
    size /= 1024;
    ++unitIdx;
  }

  char const *const fmt =
    unitIdx == 0
    // no digits after decimal point for bytes
    // because showing a fraction of a byte doesn't make sense
    ? "%.0lf %s"
    // 2 digits after decimal points for denominations
    // greater than bytes
    : "%.2lf %s";

  snprintf(out, outSize, fmt, size, units[unitIdx]);
}
