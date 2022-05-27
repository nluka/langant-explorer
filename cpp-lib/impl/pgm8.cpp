#include <string>
#include "../includes/pgm8.hpp"
#include "../includes/cstr.hpp"

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

using pgm8::Image;

Image::Image()
: m_width{0}, m_height{0}, m_pixels{nullptr}, m_maxPixelVal{0}
{}

Image::Image(std::ifstream &file) : Image::Image() {
  load(file);
}

Image::~Image() {
  delete[] m_pixels;
}

void Image::load(std::ifstream &file) {
  if (!file.is_open()) {
    throw "file closed";
  }
  if (!file.good()) {
    throw "bad file";
  }

  int kind{};

  { // determine kind of image based on magic number
    // PGMs have 2 possible magic numbers - `P2` = ASCII, `P5` = binary
    char magicNum[3]{};
    file.getline(magicNum, sizeof(magicNum));
    if (magicNum[0] != 'P' || (magicNum[1] != '2' && magicNum[1] != '5')) {
      throw "invalid magic number";
    }
    kind = static_cast<int>(cstr::to_int(magicNum[1]));
  }

  file >> m_width;
  file >> m_height;
  {
    int maxval;
    file >> maxval;
    m_maxPixelVal = static_cast<uint8_t>(maxval);
  }

  const size_t pixelCount = m_width * m_height;
  try {
    m_pixels = new uint8_t[pixelCount];
  } catch (...) {
    throw "not enough memory";
  }

  if (kind == 2) { // ASCII
    char pixel[4]{};
    for (size_t i = 0; i < pixelCount; ++i) {
      file >> pixel;
      m_pixels[i] = static_cast<uint8_t>(std::stoul(pixel));
    }
  } else { // binary
    throw "raw files not supported";
  }
}

uint_fast16_t Image::width() const {
  return m_width;
}
uint_fast16_t Image::height() const {
  return m_height;
}
uint8_t const *Image::pixels() const {
  return m_pixels;
}
size_t Image::pixelCount() const {
  return m_width * m_height;
}
uint8_t Image::maxPixelVal() const {
  return m_maxPixelVal;
}
