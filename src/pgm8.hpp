#ifndef NLUKA_PGM8_HPP
#define NLUKA_PGM8_HPP

#include <fstream>

// Module for reading and writing 8-bit PGM images.
namespace pgm8 {

enum class format : uint8_t
{
  NIL = 0,
  // Pixels stored in ASCII decimal.
  PLAIN = 2,
  // Pixels stored in binary raster.
  RAW = 5,
};

struct image_properties
{
public:
  [[nodiscard]] uint16_t get_width() const noexcept;
  [[nodiscard]] uint16_t get_height() const noexcept;
  [[nodiscard]] uint8_t get_maxval() const noexcept;
  [[nodiscard]] pgm8::format get_format() const noexcept;

  void set_width(uint16_t);
  void set_height(uint16_t);
  void set_maxval(uint8_t);
  void set_format(format);

  [[nodiscard]] size_t num_pixels() const noexcept;

  void validate() const;

private:
  uint16_t m_width = 0, m_height = 0;
  uint8_t m_maxval = 0;
  format m_fmt = format::NIL;
  bool
    m_width_set = false,
    m_height_set = false,
    m_maxval_set = false,
    m_fmt_set = false;
};

[[nodiscard]] image_properties read_properties(std::ifstream &file);

void read_pixels(
  std::ifstream &file,
  image_properties props,
  uint8_t *buffer
);

bool write(
  std::fstream &file,
  image_properties props,
  uint8_t const *pixels
);

} // namespace pgm8

#endif // NLUKA_PGM8_HPP
