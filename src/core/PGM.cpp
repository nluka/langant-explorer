#include "PGM.hpp"
#include "../util/util.hpp"

void pgm_write_file_ascii(
  std::ofstream & file,
  uint16_t const colCount,
  uint16_t const rowCount,
  color_t const maxPixelValue,
  color_t const * const pixels
) {
  file
    << "P2\n" // magic number
    << colCount << ' ' << rowCount << '\n'
    << static_cast<int>(maxPixelValue) << '\n';

  // pixels
  for (uint16_t row = 0; row < rowCount; ++row) {
    for (uint16_t col = 0; col < colCount; ++col) {
      file
        << static_cast<int>(pixels[get_one_dimensional_index(colCount, col, row)])
        << ' ';
    }
    file << '\n';
  }
}

void pgm_write_file_binary(
  std::ofstream & file,
  uint16_t const colCount,
  uint16_t const rowCount,
  color_t const maxPixelValue,
  color_t const * pixels
) {
  // header
  file
    << "P5\n" // magic number
    << colCount << ' ' << rowCount << '\n'
    << static_cast<int>(maxPixelValue) << '\n';

  // pixels
  file.write(reinterpret_cast<char const *>(pixels), colCount * rowCount);
}
