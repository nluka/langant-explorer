#ifndef NLUKA_FREGEX_H
#define NLUKA_FREGEX_H

#include <filesystem>
#include <fstream>
#include <vector>

// Module for searching the filesystem with regular expressions.
namespace fregex
{
  namespace entry_type
  {
    constexpr uint8_t block_file     = 1 << 0;
    constexpr uint8_t character_file = 1 << 1;
    constexpr uint8_t directory      = 1 << 2;
    constexpr uint8_t fifo           = 1 << 3;
    constexpr uint8_t other          = 1 << 4;
    constexpr uint8_t regular_file   = 1 << 5;
    constexpr uint8_t socket         = 1 << 6;
    constexpr uint8_t symlink        = 1 << 7;
    constexpr uint8_t all = 0b11111111;
  }

  std::vector<std::filesystem::path> find(
    std::filesystem::path const &search_path,
    char const *pattern,
    uint8_t entry_type_bit_field,
    bool recursive = false,
    std::filesystem::path const &debug_log_path = "");

  // std::vector<std::filesystem::path> iterate(
  //   std::filesystem::path const &search_path,
  //   char const *pattern,
  //   uint8_t entry_type_bit_field,
  //   std::function<void (std::filesystem::directory_entry &entry)> func,
  //   bool recursive = false,
  //   std::filesystem::path const &debug_log_path = "");

} // namespace fregex

#endif // NLUKA_FREGEX_H
