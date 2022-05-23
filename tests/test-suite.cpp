#include <fstream>
#include "../cpp-lib/includes/lengthof.hpp"
#include "../cpp-lib/includes/test.hpp"

using
  test::Assertion, test::run_suite,
  term::printf_colored, term::ColorText;

template<typename FstreamType>
void assert_file(FstreamType const *file, char const *const name) {
  if (!file->is_open()) {
    printf_colored(
      ColorText::RED,
      "failed to open file `%s`",
      name
    );
    exit(-1);
  }
}

int main() {
  term::set_color_text_default(ColorText::DEFAULT);

  std::ofstream *result = nullptr;
  { // setup result file
    std::string const pathname = "tests/out/assertions.txt";
    result = new std::ofstream(pathname);
    assert_file<std::ofstream>(result, pathname.c_str());
    test::set_ofstream(result);
  }

  {
    Assertion const assertions[] {
      Assertion(TEST_GEN_NAME((1 + 1) == 2)),
    };
    run_suite("dummy", assertions, lengthof(assertions));
  }

  return 0;
}
