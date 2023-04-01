#include <algorithm>
#include <regex>
#include <sstream>
#include <string>

#include "fregex.hpp"

namespace fs = std::filesystem;

std::vector<std::filesystem::path> fregex::find(
  std::filesystem::path const &search_path,
  char const *const pattern,
  bool const recursive,
  std::filesystem::path const &debug_log_path)
{
  if (pattern == nullptr)
    throw std::runtime_error("pattern == nullptr");
  if (std::strlen(pattern) == 0)
    throw std::runtime_error("empty pattern");
  if (!fs::is_directory(search_path))
    throw std::runtime_error("search_path is not a directory");

  std::vector<fs::path> matches{};
  std::regex const regex(pattern);
  bool const debug = !debug_log_path.empty();

  std::ofstream debug_file;
  if (debug) {
    debug_file.open(debug_log_path);
    debug_file
      << "search_path = " << search_path << '\n'
      << "pattern = /" << pattern << "/\n"
      << "files touched: \n";
  }

  auto const process_entry = [&](fs::directory_entry const &entry) {
    // ignore anything that isn't a file or symlink (shortcut)
    if (
      !entry.is_regular_file() &&
      !entry.is_block_file() &&
      !entry.is_character_file() &&
      !entry.is_fifo() &&
      !entry.is_symlink()
    ) {
      return;
    }

    if (debug) {
      debug_file << "  " << entry.path() << '\n';
    }

    std::string const filename_str = entry.path().filename().string();
    if (std::regex_match(filename_str, regex)) {
      matches.emplace_back(std::move(entry.path()));
    }
  };

  fs::directory_options const dir_opts = fs::directory_options::skip_permission_denied;

  if (recursive) {
    for (auto &entry : fs::recursive_directory_iterator(search_path, dir_opts)) {
      process_entry(entry);
    }
  } else {
    for (auto &entry : fs::directory_iterator(search_path, dir_opts)) {
      process_entry(entry);
    }
  }

  if (debug) {
    debug_file << "files matched: \n";
    for (auto const &match : matches) {
      debug_file << "  " << match << '\n';
    }
  }

  return matches;
}
