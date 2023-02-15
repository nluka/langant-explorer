#ifndef NLUKA_REGEXGLOB_H
#define NLUKA_REGEXGLOB_H

#include <filesystem>
#include <fstream>
#include <vector>

// Module for file matching using regular expressions.
namespace regexglob {

void set_preferred_separator(char);

// Converts all path separators to `sep`.
void homogenize_path_separators(std::filesystem::path &path, char const sep);
// Converts all path separators to `sep`.
void homogenize_path_separators(std::string &path, char const sep);

void set_ofstream(std::ofstream *);

// Matches any files starting from `root` (including those in subdirectories) using the `filePattern` regular expression.
std::vector<std::filesystem::path> fmatch(
  char const *root,
  char const *filePattern
);

} // namespace regexglob

#endif // NLUKA_REGEXGLOB_H
