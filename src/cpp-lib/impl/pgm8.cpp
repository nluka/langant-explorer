#include <string>
#include "../includes/pgm8.hpp"

static
void write_header(
  std::ofstream *const file,
  char const *const magicNum,
  uint16_t const width,
  uint16_t const height,
  uint8_t const maxPixelVal
) {
  if (maxPixelVal < 1) {
    throw "maxPixelVal must be > 0";
  }
  *file
    << magicNum << '\n'
    << std::to_string(width) << ' ' << std::to_string(height) << '\n'
    << std::to_string(maxPixelVal) << '\n';
}

void pgm8::write_ascii(
  std::ofstream *const file,
  uint16_t const width,
  uint16_t const height,
  uint8_t const maxPixelVal,
  uint8_t const *const pixels
) {
  write_header(file, "P2", width, height, maxPixelVal);

  // pixels
  for (uint16_t r = 0; r < height; ++r) {
    for (uint16_t c = 0; c < width; ++c) {
      *file
        << static_cast<int>(pixels[arr2d::get_1d_idx(width, c, r)])
        << ' ';
    }
    *file << '\n';
  }
}
