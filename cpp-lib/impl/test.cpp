#include <iostream>
#include "../includes/cstr.hpp"
#include "../includes/test.hpp"

using
  test::Assertion,
  term::printf_colored, term::ColorText;

static bool s_useStdout = true;
void test::use_stdout(bool const boolean) {
  s_useStdout = boolean;
}

static std::ofstream *s_ofstream = nullptr;
void test::set_ofstream(std::ofstream *const ofs) {
  s_ofstream = ofs;
}

size_t
  Assertion::s_successCount = 0,
  Assertion::s_failCount = 0;
size_t Assertion::get_success_count() {
  return s_successCount;
}
size_t Assertion::get_fail_count() {
  return s_failCount;
}
void Assertion::reset_counters() {
  s_successCount = 0;
  s_failCount = 0;
}

void Assertion::print_summary() {
  if (s_failCount == 0) {
    if (s_useStdout) {
      printf_colored(
        ColorText::GREEN,
        "all %zu assertions passed\n\n",
        s_successCount
      );
    }
    if (s_ofstream != nullptr) {
      *s_ofstream
        << "all " << s_successCount
        << " assertions passed\n\n";
    }
  } else {
    if (s_useStdout) {
      printf_colored(
        ColorText::YELLOW,
        "%zu assertions failed, %zu passed\n\n",
        s_failCount, s_successCount
      );
    }
    if (s_ofstream != nullptr) {
      *s_ofstream
        << s_failCount << " assertions failed, "
        << s_successCount << " passed\n\n";
    }
  }
}

Assertion::Assertion(char const *const name, bool const expr)
: m_name{name}, m_expr{expr}
{}
Assertion::Assertion(std::string const name, bool const expr)
: m_name{name}, m_expr{expr}
{}

void Assertion::run(bool const verbose) const {
  if (m_expr == true) {
    ++s_successCount;

    if (s_useStdout && verbose) {
      printf_colored(
        ColorText::GREEN,
        "`%s` passed\n",
        m_name.c_str()
      );
    }

    if (s_ofstream != nullptr && verbose) {
      *s_ofstream << "`" << m_name << "` passed\n";
    }
  } else {
    ++s_failCount;

    if (s_useStdout) {
      printf_colored(
        ColorText::RED,
        "`%s` failed\n",
        m_name.c_str()
      );
    }

    if (s_ofstream != nullptr) {
      *s_ofstream << "`" << m_name << "` failed\n";
    }
  }
}

template<typename LambdaT>
void suite_runner(
  char const *const name,
  LambdaT const &lambda
) {
  if (s_useStdout) {
    term::printf_colored(term::ColorText::MAGENTA, "=== %s ===\n", name);
  }
  if (s_ofstream != nullptr) {
    *s_ofstream << "=== " << name << " ===\n";
  }

  lambda();

  Assertion::print_summary();
  Assertion::reset_counters();
}

void test::run_suite(
  char const *const name,
  Assertion const assertions[],
  size_t const assertionCount
) {
  suite_runner(name, [&assertions, assertionCount]() {
    for (size_t i = 0; i < assertionCount; ++i) {
      assertions[i].run();
    }
  });
}

void test::run_suite(
  char const *const name,
  std::vector<Assertion> const &assertions
) {
  suite_runner(name, [&assertions]() {
    for (auto const &asr : assertions) {
      asr.run();
    }
  });
}

void test::print_newline() {
  if (s_useStdout) {
    std::cout << '\n';
  }
  if (s_ofstream != nullptr) {
    *s_ofstream << '\n';
  }
}
