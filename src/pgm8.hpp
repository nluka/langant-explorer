#ifndef NLUKA_PGM8_HPP
#define NLUKA_PGM8_HPP

#include <fstream>

// Module for reading and writing 8-bit PGM images.
namespace pgm8 {

struct image
{
  uint16_t width, height;
  uint8_t maxval;
  uint8_t *pixels;
};

enum class format
{
  // Pixels stored in ASCII decimal.
  PLAIN = 2,
  // Pixels stored in binary raster.
  RAW = 5,
};

image read(std::ifstream &file);

void write(
  std::ofstream &file,
  pgm8::image const &img,
  pgm8::format const fmt
);

void write(
  std::ofstream &file,
  uint16_t const width,
  uint16_t const height,
  uint8_t const maxval,
  uint8_t const *const pixels,
  pgm8::format const fmt
);

} // namespace pgm8

#endif // NLUKA_PGM8_HPP
