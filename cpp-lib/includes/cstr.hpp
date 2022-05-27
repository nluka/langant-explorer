#ifndef CPPLIB_CSTR_HPP
#define CPPLIB_CSTR_HPP

#include <cstdlib>

namespace cstr { // stands for `C string`

constexpr
int cmp(char const *s1, char const *s2) {
  // from https://stackoverflow.com/a/34873406/16471560
  while(*s1 && (*s1 == *s2)) {
    ++s1;
    ++s2;
  }
  return *(unsigned char const *)s1 - *(unsigned char const *)s2;
}

constexpr
// Returns the number of occurences of a character.
size_t count(char const *const str, char const c) {
  size_t i = 0, count = 0;
  while (str[i] != '\0') {
    if (str[i] == c) {
      ++count;
    }
    ++i;
  }
  return count;
}

constexpr
size_t len(char const *const str) {
  size_t i = 0;
  while (str[i] != '\0') {
    ++i;
  }
  return i;
}

constexpr
// Returns the last character of the given string.
char last_char(char const *const str) {
  size_t const slen = len(str);
  return slen > 0 ? str[slen - 1] : '\0';
}

constexpr
// Converts an ASCII number ('1') to an integer (1).
int to_int(char const c) {
  return static_cast<int>(c) - 48;
}

} // namespace cstr

#endif // CPPLIB_CSTR_HPP
