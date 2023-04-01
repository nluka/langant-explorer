#ifndef NLUKA_FREGEX_H
#define NLUKA_FREGEX_H

#include <filesystem>
#include <fstream>
#include <vector>

// Module for searching the filesystem with regular expressions.
namespace fregex {

std::vector<std::filesystem::path> find(
  std::filesystem::path const &search_path,
  char const *pattern,
  bool recursive = false,
  std::filesystem::path const &debug_log_path = "");

} // namespace fregex

#endif // NLUKA_FREGEX_H
