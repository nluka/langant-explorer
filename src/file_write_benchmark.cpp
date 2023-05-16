#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <cassert>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>

#include "primitives.hpp"
#include "scoped_timer.hpp"

int main() {
  u64 width = 10'000;
  u64 height = 10'000;
  u64 num_pixels = width * height;

  std::vector<u8> pixels{};

  srand(u32(time(NULL)));

  {
    scoped_timer<scoped_timer_unit::MILLISECONDS> timer("generating random pixel data", std::cout);

    pixels.reserve(num_pixels);

    for (u64 i = 0; i < num_pixels; ++i)
      pixels.push_back((uint8_t)rand());
  }

  // FILE
  {
    FILE *file = fopen("stdio_FILE.bin", "wb");
    assert(file);
    {
      scoped_timer<scoped_timer_unit::MICROSECONDS> timer("stdio FILE", std::cout);
      assert(fwrite(pixels.data(), pixels.size(), sizeof(u8), file));
    }
    fclose(file);
    std::filesystem::remove("stdio_FILE.bin");
  }

  // std::fstream
  {
    std::fstream file("std_fstream.bin", std::ios::binary | std::ios::out);
    assert(file);
    {
      scoped_timer<scoped_timer_unit::MICROSECONDS> timer("std::fstream", std::cout);
      file.write((char const *)pixels.data(), std::streamsize(pixels.size()));
    }
    std::filesystem::remove("std_fstream.bin");
  }

  // std::fstream
  {
    std::ofstream file("std_ofstream.bin", std::ios::binary);
    assert(file);
    {
      scoped_timer<scoped_timer_unit::MICROSECONDS> timer("std::ofstream", std::cout);
      file.write((char const *)pixels.data(), std::streamsize(pixels.size()));
    }
    std::filesystem::remove("std_ofstream.bin");
  }

  // linux write
  {
    int fd = open("linux_write.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    {
      scoped_timer<scoped_timer_unit::MICROSECONDS> timer("linux write", std::cout);
      assert(write(fd, pixels.data(), pixels.size()) == i64(pixels.size()));
    }
    close(fd);
    std::filesystem::remove("linux_write.bin");
  }
}