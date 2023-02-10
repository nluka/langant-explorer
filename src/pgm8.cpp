#pragma warning(push, 0)
#include <string>
#include <sstream>
#pragma warning(pop)

#include "pgm8.hpp"

uint16_t pgm8::image_properties::get_width() const noexcept { return m_width; }
uint16_t pgm8::image_properties::get_height() const noexcept { return m_height; }
uint8_t pgm8::image_properties::get_maxval() const noexcept { return m_maxval; }
pgm8::format pgm8::image_properties::get_format() const noexcept { return m_fmt; }

template <typename Ty>
void ensure_greater_than_zero(Ty v, char const *const name)
{
  if (v == 0) {
    std::stringstream err{};
    err << name << " must be > 0";
    throw std::runtime_error(err.str());
  }
}

static
void ensure_legal_format(pgm8::format const v)
{
  if (v != pgm8::format::PLAIN && v != pgm8::format::RAW)
    throw std::runtime_error("illegal format, must be PLAIN (2) or RAW (5)");
}

void pgm8::image_properties::set_width(uint16_t const v)
{
  ensure_greater_than_zero(v, "width");
  m_width = v;
  m_width_set = true;
}
void pgm8::image_properties::set_height(uint16_t const v)
{
  ensure_greater_than_zero(v, "height");
  m_height = v;
  m_height_set = true;
}
void pgm8::image_properties::set_maxval(uint8_t const v)
{
  ensure_greater_than_zero(v, "maxval");
  m_maxval = v;
  m_maxval_set = true;
}
void pgm8::image_properties::set_format(pgm8::format const v)
{
  ensure_legal_format(v);
  m_fmt = v;
  m_fmt_set = true;
}

size_t pgm8::image_properties::num_pixels() const noexcept
{
  return static_cast<size_t>(m_width) * m_height;
}

void pgm8::image_properties::validate() const
{
  if (!m_width_set) throw std::runtime_error("width not set");
  if (!m_height_set) throw std::runtime_error("height not set");
  if (!m_maxval_set) throw std::runtime_error("maxval not set");
  if (!m_fmt_set) throw std::runtime_error("format not set");
}

pgm8::image_properties pgm8::read_properties(std::ifstream &file)
{
  if (!file.is_open())
    throw std::runtime_error("file not open");
  if (!file.good())
    throw std::runtime_error("file not in good state");

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
      throw std::runtime_error("invalid magic number, corrupt or non-PGM file");
  }();

  uint16_t width, height;
  file >> width >> height;

  uint8_t maxval;
  {
    int maxval_;
    file >> maxval_;
    maxval = static_cast<uint8_t>(maxval_);
  }

  image_properties props;
  props.set_width(width);
  props.set_height(height);
  props.set_maxval(maxval);
  props.set_format(fmt);

  return props;
}

void pgm8::read_pixels(
  std::ifstream &file,
  image_properties const props,
  uint8_t *const buffer)
{
  // eat the \n between maxval and pixel data
  {
    char newline;
    file.read(&newline, 1);
  }

  size_t const num_pixels = static_cast<size_t>(props.get_width()) * props.get_height();

  if (props.get_format() == pgm8::format::RAW)
  {
    file.read(reinterpret_cast<char *>(buffer), num_pixels);
  }
  else // format::PLAIN
  {
    char pixel[4] {};
    for (size_t i = 0; i < num_pixels; ++i) {
      file >> pixel;
      buffer[i] = static_cast<uint8_t>(std::stoul(pixel));
    }
  }
}

void pgm8::write(
  std::ofstream &file,
  image_properties const props,
  uint8_t const *pixels)
{
  props.validate();

  uint16_t const width = props.get_width(), height = props.get_height();
  uint8_t const maxval = props.get_maxval();
  format const fmt = props.get_format();

  ensure_greater_than_zero(props.get_width(), "width");
  ensure_greater_than_zero(props.get_height(), "height");
  ensure_greater_than_zero(props.get_maxval(), "maxval");
  ensure_legal_format(fmt);

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
