#include <iostream>
#include <filesystem>
#include <cinttypes>

#include "fregex.hpp"
#include "primitives.hpp"
#include "util.hpp"

i32 main(i32 const argc, char const *const *const argv) {
  if (argc < 2 || argc > 3) {
    std::cout <<
      "\n"
      "Usage:\n"
      "  next_cluster <directory> [print_num]\n"
      "                            ^^^^^^^^^\n"
      "                            Y|y|1\n"
      "\n";
    return 0;
  }

  char const *const search_dir_cstr = argv[1];
  std::filesystem::path const search_dir = search_dir_cstr;

  if (!std::filesystem::exists(search_dir)) {
    util::print_err("path '%s' does not exist", search_dir_cstr);
    return 0;
  }
  if (!std::filesystem::is_directory(search_dir)) {
    util::print_err("path '%s' is not a directory", search_dir_cstr);
    return 0;
  }

  auto const clusters = fregex::find(
    search_dir,
    "^cluster[0-9]+$",
    fregex::entry_type::directory);

  auto const exit = [&](i32 const code) {
    if (argc == 3) {
      std::string const print_num_val = argv[2];
      if (print_num_val == "Y" || print_num_val == "y" || print_num_val == "1") {
        std::cout << code << '\n';
      }
    }
    std::exit(code);
  };

  if (clusters.size() >= INT32_MAX - 1) {
    exit(0);
  }
  if (clusters.empty()) {
    exit(1);
  }

  i32 max_cluster_num = 0;
  for (auto const &cluster : clusters) {
    char const *const cluster_num_cstr = std::strpbrk(cluster.string().c_str(), "0123456789");
    if (cluster_num_cstr != nullptr) {
      i32 const cluster_num = std::stoi(cluster_num_cstr);
      if (cluster_num > max_cluster_num)
        max_cluster_num = cluster_num;
    }
  }

  exit(max_cluster_num + 1);
}
