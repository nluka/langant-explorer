#ifndef CPPLIB_PGM8_HPP
#define CPPLIB_PGM8_HPP

#include <fstream>
#include <cinttypes>
#include <string>
#include "arr2d.hpp"

namespace pgm8 { // stands for `portable gray map 8-bit`

void write_ascii(
  std::ofstream *file,
  uint16_t width,
  uint16_t height,
  uint8_t maxPixelVal,
  uint8_t const *pixels
);

class Image {
private:
  uint_fast16_t m_width, m_height;
  uint8_t *m_pixels, m_maxPixelVal;

public:
  Image();
  Image(std::ifstream &file);
  ~Image();
  void load(std::ifstream &file);
  uint_fast16_t width() const;
  uint_fast16_t height() const;
  uint8_t const *pixels() const;
  size_t pixelCount() const;
  uint8_t maxPixelVal() const;
};

} // namespace pgm

#endif // CPPLIB_PGM8_HPP
