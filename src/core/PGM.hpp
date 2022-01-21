#ifndef ANT_SIMULATOR_LITE_CORE_PGM_HPP
#define ANT_SIMULATOR_LITE_CORE_PGM_HPP

#include <fstream>
#include "types.hpp"

void pgm_write_file_ascii(
  std::ofstream & file,
  uint16_t colCount,
  uint16_t rowCount,
  color_t maxPixelValue,
  color_t const * pixels
);

void pgm_write_file_binary(
  std::ofstream & file,
  uint16_t colCount,
  uint16_t rowCount,
  color_t maxPixelValue,
  color_t const * pixels
);

#endif // ANT_SIMULATOR_LITE_CORE_PGM_HPP
