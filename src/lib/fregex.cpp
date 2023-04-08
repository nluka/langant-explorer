#include <cassert>
#include <algorithm>
#include <regex>
#include <sstream>
#include <string>

#include "fregex.hpp"

namespace fs = std::filesystem;

std::vector<std::filesystem::path> fregex::find(
  std::filesystem::path const &search_path,
  char const *const pattern,
  uint8_t const entry_type_bit_field,
  bool const recursive,
  std::filesystem::path const &debug_log_path)
{
  assert(pattern != nullptr);
  assert(std::strlen(pattern) > 0);
  assert(fs::is_directory(search_path));

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
    if (
      ((entry_type_bit_field & fregex::entry_type::block_file) && !entry.is_block_file()) &&
      ((entry_type_bit_field & fregex::entry_type::character_file) && !entry.is_character_file()) &&
      ((entry_type_bit_field & fregex::entry_type::directory) && !entry.is_directory()) &&
      ((entry_type_bit_field & fregex::entry_type::fifo) && !entry.is_fifo()) &&
      ((entry_type_bit_field & fregex::entry_type::other) && !entry.is_other()) &&
      ((entry_type_bit_field & fregex::entry_type::other) && !entry.is_other()) &&
      ((entry_type_bit_field & fregex::entry_type::regular_file) && !entry.is_regular_file()) &&
      ((entry_type_bit_field & fregex::entry_type::socket) && !entry.is_socket()) &&
      ((entry_type_bit_field & fregex::entry_type::symlink) && !entry.is_symlink())
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
