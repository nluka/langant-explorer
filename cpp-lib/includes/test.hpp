#ifndef CPPLIB_TEST_HPP
#define CPPLIB_TEST_HPP

#include <fstream>
#include <vector>
#include "term.hpp"

namespace test {

#define TEST_GEN_NAME(asr) #asr, asr

void use_stdout(bool boolean);
void set_ofstream(std::ofstream *ofs);

class Assertion {
private:
  static size_t s_successCount, s_failCount;

  char const *const m_name;
  bool const m_expr;

public:
  static size_t get_success_count();
  static size_t get_fail_count();
  static void print_summary();
  static void reset_counters();

  Assertion(char const *name, bool expr);
  void run(bool verbose = false) const;
};

void run_suite(
  char const *name,
  Assertion const assertions[],
  size_t const assertionCount
);
void run_suite(
  char const *name,
  std::vector<Assertion> const &assertions
);

void print_newline();

} // namespace test

#endif // CPPLIB_TEST_HPP
