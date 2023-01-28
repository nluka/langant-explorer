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
