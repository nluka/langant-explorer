#include <algorithm>
#include <regex>
#include <sstream>
#include <string>

#include "regexglob.hpp"

namespace fs = std::filesystem;

static char s_prefSep = '/';
void regexglob::set_preferred_separator(char const c) {
  s_prefSep = c;
}

void regexglob::homogenize_path_separators(
  fs::path &path,
  char const sep
) {
  std::string homogenized = path.string();

  std::replace_if(
    homogenized.begin(),
    homogenized.end(),
    [](char const c) {
      return c == '/' || c == '\\';
    },
    sep
  );

  path = homogenized;
}
void regexglob::homogenize_path_separators(
  std::string &path,
  char const sep
) {
  std::replace_if(
    path.begin(),
    path.end(),
    [](char const c) {
      return c == '/' || c == '\\';
    },
    sep
  );
}

static std::ofstream *s_ofstream = nullptr;
void regexglob::set_ofstream(std::ofstream *const ofs) {
  s_ofstream = ofs;
}

std::vector<fs::path> regexglob::fmatch(
  char const *const root,
  char const *const filePattern
) {
  fs::path const rootDir(root);
  if (!fs::is_directory(rootDir)) {
    throw "`root` is not a directory";
  }

  if (std::strlen(filePattern) == 0) {
    throw "blank `filePattern`";
  }

  std::vector<fs::path> matchedFiles{};
  std::regex const regex(filePattern);

  if (s_ofstream != nullptr) {
    *s_ofstream
      << "<root>: " << root << '\n'
      << "<file_pattern>: " << filePattern << '\n'
      << "<files_checked>:\n";
  }

  for (
    auto const &entry :
    fs::recursive_directory_iterator(
      rootDir,
      fs::directory_options::skip_permission_denied
    )
  ) {
    // ignore anything that isn't a file or shortcut
    if (
      !entry.is_regular_file() &&
      !entry.is_block_file() &&
      !entry.is_character_file() &&
      !entry.is_fifo() &&
      !entry.is_symlink()
    ) {
      continue;
    }

    if (s_ofstream != nullptr) {
      *s_ofstream << "  " << entry.path() << '\n';
    }

    {
      std::string path = entry.path().string();
      std::string const fileName = entry.path().filename().string();
      if (std::regex_match(fileName, regex)) {
        regexglob::homogenize_path_separators(path, s_prefSep);
        matchedFiles.emplace_back(path);
      }
    }
  }

  if (s_ofstream != nullptr) {
    *s_ofstream << "<files_matched>:";
    if (matchedFiles.empty()) {
      *s_ofstream << " none\n";
    } else {
      *s_ofstream << '\n';
      for (auto const &match : matchedFiles) {
        *s_ofstream << "  " << match << '\n';
      }
    }
    *s_ofstream << '\n';
  }

  return matchedFiles;
}
