#include <string>

#include "pgm8.hpp"

pgm8::image pgm8::read(std::ifstream &file)
{
  if (!file.is_open())
    throw std::runtime_error("pgm8::read error - `file` not open");
  if (!file.good())
    throw std::runtime_error("pgm8::read error - `file` not in good state");

  format const fmt = [&file]()
  {
    auto const string_starts_with = [](
      std::string const &subject,
      char const *const sequence)
    {
      size_t const seq_len = std::strlen(sequence);

      if (subject.length() < seq_len)
        return false;

      for (size_t i = 0; i < seq_len; ++i)
        if (subject[i] != sequence[i])
          return false;

      return true;
    };

    std::string magic_num{};
    std::getline(file, magic_num);
    if (string_starts_with(magic_num, "P5"))
      return format::RAW;
    else if (string_starts_with(magic_num, "P2"))
      return format::PLAIN;
    else
      throw std::runtime_error("pgm8::read error - invalid magic number");
  }();

  if (fmt != format::PLAIN && fmt != format::RAW)
    throw std::runtime_error("pgm8::read error - bad `format`");

  uint16_t width, height;
  file >> width >> height;

  uint8_t maxval;
  {
    int maxval_;
    file >> maxval_;
    maxval = static_cast<uint8_t>(maxval_);
  }

  // eat the \n between maxval and pixel data
  {
    char newline;
    file.read(&newline, 1);
  }

  size_t const num_pixels = static_cast<size_t>(width) * height;
  uint8_t *const pixels = new uint8_t[num_pixels];

  if (fmt == format::RAW)
  {
    file.read(reinterpret_cast<char *>(pixels), num_pixels);
  }
  else // format::PLAIN
  {
    char pixel[4] {};
    for (size_t i = 0; i < num_pixels; ++i) {
      file >> pixel;
      pixels[i] = static_cast<uint8_t>(std::stoul(pixel));
    }
  }

  return {
    width,
    height,
    maxval,
    pixels,
  };
}

void pgm8::write(
  std::ofstream &file,
  pgm8::image const &img,
  pgm8::format const fmt)
{
  pgm8::write(file, img.width, img.height, img.maxval, img.pixels, fmt);
}

void pgm8::write(
  std::ofstream &file,
  uint16_t const width,
  uint16_t const height,
  uint8_t const maxval,
  uint8_t const *const pixels,
  pgm8::format const fmt)
{
  if (maxval < 1)
    throw std::runtime_error("pgm8::write error - `maxval` must be > 0");
  if (fmt != format::PLAIN && fmt != format::RAW)
    throw std::runtime_error("pgm8::write error - bad `format`");

  // header
  {
    int const magic_num = (fmt == format::RAW) ? 5 : /* format::PLAIN */ 2;
    file
      << 'P' << magic_num << '\n'
      << std::to_string(width) << ' ' << std::to_string(height) << '\n'
      << std::to_string(maxval) << '\n';
  }

  // pixels
  if (fmt == format::RAW)
  {
    size_t const num_pixels = static_cast<size_t>(width) * height;
    file.write(reinterpret_cast<char const *>(pixels), num_pixels);
  }
  else // format::PLAIN
  {
    for (size_t r = 0; r < height; ++r)
    {
      for (size_t c = 0; c < width; ++c)
        file << std::to_string(pixels[(r * width) + c]) << ' ';
      file << '\n';
    }
  }
}
