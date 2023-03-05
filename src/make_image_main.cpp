#include <random>
#include <regex>

#include <boost/program_options.hpp>
#include "lib/pgm8.hpp"

#include "primitives.hpp"
#include "util.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

typedef std::uniform_int_distribution<usize> usize_distrib;

#define OPT_FMT_FULL "fmt"
#define OPT_FMT_SHORT "f"
#define REGEX_FMT "^(rawPGM)|(plainPGM)$"

#define OPT_OUTPATH_FULL "outpath"
#define OPT_OUTPATH_SHORT "p"

#define OPT_CONTENT_FULL "content"
#define OPT_CONTENT_SHORT "c"
#define REGEX_CONTENT "^(noise)|(fill=[0-9]{1,3})$"

#define OPT_WIDTH_FULL "width"
#define OPT_WIDTH_SHORT "w"

#define OPT_HEIGHT_FULL "height"
#define OPT_HEIGHT_SHORT "h"

#define OPT_MAXVAL_FULL "maxval"
#define OPT_MAXVAL_SHORT "m"

struct config
{
  usize       width;
  usize       height;
  usize       maxval;
  std::string format;
  std::string content;
  fs::path    path;
  u8          fill_value;
};

static config s_cfg{};

errors_t parse_config(int const argc, char const *const *argv);
bpo::options_description options_description();
std::string usage_msg();

int main(int const argc, char const *const *const argv)
{
  sizeof(std::string);
  sizeof(fs::path);
  if (argc == 1) {
    std::cout << usage_msg();
    std::exit(1);
  }

  {
    errors_t const errors = parse_config(argc, argv);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err(err.c_str());
      die("%zu configuration errors", errors.size());
    }
  }

  usize const num_pixels = s_cfg.width * s_cfg.height;
  auto pixels = std::make_unique<u8[]>(num_pixels);

  if (s_cfg.content.starts_with("fill")) {
    std::fill_n(pixels.get(), num_pixels, s_cfg.fill_value);
  } else {
    // noise...

    std::random_device rand_device;
    std::mt19937 rand_num_gener(rand_device());
    usize_distrib const pixel_distribution(0, s_cfg.maxval);

    for (usize i = 0; i < num_pixels; ++i) {
      auto const rand_pixel = pixel_distribution(rand_num_gener);
      pixels.get()[i] = static_cast<u8>(rand_pixel);
    }
  }

  if (s_cfg.format.ends_with("PGM")) {
    pgm8::format const pgm_fmt = s_cfg.format.starts_with("raw")
      ? pgm8::format::RAW
      : pgm8::format::PLAIN;

    pgm8::image_properties img_props;
    img_props.set_format(pgm_fmt);
    // parse_config validates that the width, height, and maxval are
    // in their respective ranges, so casting is OK...
    img_props.set_width(static_cast<u16>(s_cfg.width));
    img_props.set_height(static_cast<u16>(s_cfg.height));
    img_props.set_maxval(static_cast<u8>(s_cfg.maxval));

    // std::ios_base::openmode fmode = std::ios::out;
    // if (pgm_fmt == pgm8::format::RAW) {
    //   fmode |= std::ios::binary;
    // }

    std::fstream file(s_cfg.path, std::ios::out);

    try {
      pgm8::write(file, img_props, pixels.get());
    } catch (std::runtime_error const &except) {
      die(except.what());
    }
  } else {
    die("implementation for specified image format does not exist");
  }

  return 0;
}

std::string usage_msg()
{
  std::ostringstream msg;

  msg <<
    "\n"
    "USAGE: \n"
    "  make_image [options] \n"
    "\n"
  ;

  options_description().print(msg, 6);

  msg << '\n';

  return msg.str();
}

bpo::options_description options_description()
{
  bpo::options_description desc("REQUIRED OPTIONS");

  desc.add_options()
    (OPT_FMT_FULL "," OPT_FMT_SHORT, bpo::value<std::string>(), "Image type, format /" REGEX_FMT "/")
    (OPT_OUTPATH_FULL "," OPT_OUTPATH_SHORT, bpo::value<std::string>(), "Path of output image file")
    (OPT_CONTENT_FULL "," OPT_CONTENT_SHORT, bpo::value<std::string>(), "Type of image content, format /" REGEX_CONTENT "/")
    (OPT_WIDTH_FULL "," OPT_WIDTH_SHORT, bpo::value<usize>(), "Image width, [1, 65535]")
    (OPT_HEIGHT_FULL "," OPT_HEIGHT_SHORT, bpo::value<usize>(), "Image height, [1, 65535]")
    (OPT_MAXVAL_FULL "," OPT_MAXVAL_SHORT, bpo::value<usize>(), "Maximum pixel value, [1, 255]")
  ;

  return desc;
}

errors_t parse_config(int const argc, char const *const *const argv)
{
  using util::get_required_option;
  using util::get_nonrequired_option;
  using util::make_str;

  errors_t errors{};

  bpo::variables_map var_map;
  try {
    bpo::store(bpo::parse_command_line(argc, argv, options_description()), var_map);
  } catch (std::exception const &err) {
    errors.emplace_back(err.what());
    return errors;
  }
  bpo::notify(var_map);

  {
    auto format = get_required_option<std::string>(OPT_FMT_FULL, OPT_FMT_SHORT, var_map, errors);

    if (format.has_value()) {
      if (!std::regex_match(format.value(), std::regex(REGEX_FMT))) {
        errors.emplace_back("(--" OPT_FMT_FULL ", -" OPT_FMT_SHORT ") must match /" REGEX_FMT "/");
      } else {
        s_cfg.format = std::move(format.value());
      }
    }
  }
  {
    auto out_path = get_required_option<std::string>(OPT_OUTPATH_FULL, OPT_OUTPATH_SHORT, var_map, errors);

    if (out_path.has_value()) {
      s_cfg.path = std::move(out_path.value());
    }
  }
  {
    auto content = get_required_option<std::string>(OPT_CONTENT_FULL, OPT_CONTENT_SHORT, var_map, errors);

    if (content.has_value()) {
      if (std::regex_match(content.value(), std::regex(REGEX_CONTENT))) {
        if (content.value().starts_with("fill")) {
          usize const equals_sign_pos = content.value().find_first_of('=');
          std::string const fill_val_str = content.value().substr(equals_sign_pos + 1);
          usize const fill_val = std::stoull(fill_val_str);
          if (fill_val > UINT8_MAX) {
            errors.emplace_back(make_str("(--" OPT_CONTENT_FULL ", -" OPT_CONTENT_SHORT ") fill value must be <= %zu", UINT8_MAX));
          } else {
            s_cfg.fill_value = static_cast<u8>(fill_val);
          }
        }
        s_cfg.content = std::move(content.value());
      } else {
        errors.emplace_back("(--" OPT_CONTENT_FULL ", -" OPT_CONTENT_SHORT ") must match /" REGEX_CONTENT "/");
      }
    }
  }
  {
    auto const width = get_required_option<usize>(OPT_WIDTH_FULL, OPT_WIDTH_SHORT, var_map, errors);

    if (width.has_value()) {
      if (width.value() < 1 || width.value() > UINT16_MAX) {
        errors.emplace_back(make_str("(--" OPT_WIDTH_FULL ", -" OPT_WIDTH_SHORT ") not in range [1, %zu]", UINT16_MAX));
      } else {
        s_cfg.width = width.value();
      }
    }
  }
  {
    auto const height = get_required_option<usize>(OPT_HEIGHT_FULL, OPT_HEIGHT_SHORT, var_map, errors);

    if (height.has_value()) {
      if (height.value() < 1 || height.value() > UINT16_MAX) {
        errors.emplace_back(make_str("(--" OPT_HEIGHT_FULL ", -" OPT_HEIGHT_SHORT ") not in range [1, %zu]", UINT16_MAX));
      } else {
        s_cfg.height = height.value();
      }
    }
  }
  {
    auto const maxval = get_required_option<usize>(OPT_MAXVAL_FULL, OPT_MAXVAL_SHORT, var_map, errors);

    if (maxval.has_value()) {
      if (maxval.value() < 1 || maxval.value() > UINT8_MAX) {
        errors.emplace_back(make_str("(--" OPT_MAXVAL_FULL ", -" OPT_MAXVAL_SHORT ") not in range [1, %zu]", UINT8_MAX));
      } else {
        s_cfg.maxval = maxval.value();
      }
    }
  }

  return std::move(errors);
}
