#include <random>
#include <regex>
#include <filesystem>

#include <boost/program_options.hpp>

#include "pgm8.hpp"
#include "primitives.hpp"
#include "util.hpp"
#include "program_options.hpp"

namespace fs = std::filesystem;
namespace bpo = boost::program_options;
using util::errors_t;
using util::make_str;
using util::print_err;
using util::die;

static po::make_image_options s_options{};

i32 main(i32 const argc, char const *const *const argv)
try
{
  if (argc == 1) {
    std::ostringstream usage_msg;
    usage_msg <<
      "\n"
      "Usage:\n"
      "  make_image [options]\n"
      "\n";
    po::make_image_options_description().print(usage_msg, 6);
    usage_msg << '\n';
    std::cout << usage_msg.str();
    return 1;
  }

  {
    errors_t errors{};
    po::parse_make_image_options(argc, argv, s_options, errors);

    if (!errors.empty()) {
      for (auto const &err : errors)
        print_err("%s", err.c_str());
      return 1;
    }
  }

  u64 const num_pixels = s_options.width * s_options.height;
  auto pixels = std::make_unique<u8[]>(num_pixels);

  if (s_options.content.starts_with("fill")) {
    std::fill_n(pixels.get(), num_pixels, static_cast<u8>(s_options.fill_value));
  } else {
    // noise...

    std::random_device rand_device;
    std::mt19937 rand_num_gener(rand_device());
    std::uniform_int_distribution<u16> pixel_distribution(0, s_options.maxval);

    for (u64 i = 0; i < num_pixels; ++i) {
      auto const rand_pixel = pixel_distribution(rand_num_gener);
      pixels.get()[i] = static_cast<u8>(rand_pixel);
    }
  }

  {
    pgm8::image_properties img_props;
    img_props.set_format(s_options.format);
    img_props.set_width(s_options.width);
    img_props.set_height(s_options.height);
    img_props.set_maxval(s_options.maxval);

    std::ios_base::openmode fmode = std::ios::out;
    if (s_options.format == pgm8::format::RAW) {
      fmode |= std::ios::binary;
    }

    std::fstream file(s_options.out_file_path, fmode);

    try {
      pgm8::write(file, img_props, pixels.get());
    } catch (std::runtime_error const &except) {
      die(except.what());
    }
  }

  return 0;
}
catch (std::exception const &except)
{
  die("%s", except.what());
}
catch (...)
{
  die("unknown error - catch (...)");
}
