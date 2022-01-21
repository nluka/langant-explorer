#include <filesystem>
#include "util.hpp"

void string_fix_path_separators(std::string & str) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '/' || str[i] == '\\') {
      str[i] = std::filesystem::path::preferred_separator;
    }
  }
}
