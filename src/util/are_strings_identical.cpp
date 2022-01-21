#include <string.h>
#include "util.hpp"

bool are_strings_identical(char const * const str1, char const * const str2) {
  for (size_t i = 0; ; ++i) {
    if (str1[i] != str2[i]) {
      return false;
    }
    if (str1[i] == 0 || str2[i] == 0) {
      return true;
    }
  }
}
