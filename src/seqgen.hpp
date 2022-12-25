#ifndef CPPLIB_SEQUENCEGEN_HPP
#define CPPLIB_SEQUENCEGEN_HPP

#include <concepts>
#include <cinttypes>
#include <cstring>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#include "cstr.hpp"

// Module for generating long, semi-complex integer sequences using a simple and
// concise syntax.
namespace seqgen {

// Populates a block of memory using a pattern. The caller is responsible in
// ensuring `out` has enough room for the sequence described by `pattern`.
template <typename Ty>
requires std::integral<Ty> && (std::is_same<Ty, bool>::value == false)
void populate(Ty *const out, char const *const pattern) {
  if (cstr::len(pattern) == 0) {
    throw std::runtime_error("empty pattern");
  }

  auto const pattern_ = [pattern](){
    size_t const len = cstr::len(pattern) + 1; // +1 for NUL
    char *buf = new char[len];
    std::strncpy(buf, pattern, len);
    return std::unique_ptr<char []>(buf);
  }();

  cstr::remove_spaces(pattern_.get());

  if (cstr::len(pattern_.get()) == 0) {
    throw std::runtime_error("empty pattern");
  }

  std::regex const singleNum("^-?[0-9]+$");
  std::regex const repeatedNum("^-?[0-9]+\\{[1-9][0-9]*\\}$");
  std::regex const range("^-?[0-9]+\\.\\.-?[0-9]+$");

  size_t pos = 0;

  char *piece = std::strtok(pattern_.get(), ",");
  while (piece != nullptr) {
    auto const parseNum = [](char const *const str){
      if constexpr (std::is_signed_v<Ty>) {
        // TODO: maybe replace with std::strtoimax?
        return static_cast<Ty>(std::stoll(str));
      } else {
        // TODO: maybe replace with std::strtoumax?
        return static_cast<Ty>(std::stoull(str));
      }
    };

    // process piece
    if (std::regex_match(piece, singleNum)) {
      Ty const num = parseNum(piece);
      out[pos++] = num;
    } else if (std::regex_match(piece, repeatedNum)) {
      Ty const num = parseNum(piece);

      char const *const pieceRepeats = std::find(
        piece,
        piece + cstr::len(piece),
        '{'
      ) + 1;

      size_t const repeats = std::stoull(pieceRepeats);

      for (size_t i = 1; i <= repeats; ++i) {
        out[pos++] = num;
      }
    } else if (std::regex_match(piece, range)) {
      Ty const leftNum = parseNum(piece);

      // creating this variable so we can use `find_last_of` in the next stmt
      std::string_view pieceView(piece);

      char const *const pieceRight = piece + pieceView.find_last_of('.') + 1;

      Ty const rightNum = parseNum(pieceRight);

      if (bool const ascending = leftNum < rightNum) {
        for (Ty n = leftNum; n <= rightNum; ++n) {
          out[pos++] = n;
        }
      } else { // descending
        for (Ty n = leftNum; n >= rightNum; --n) {
          out[pos++] = n;
        }
      }
    } else {
      std::stringstream err{};
      err << "piece `" << piece << "` has invalid syntax";
      throw std::runtime_error(err.str());
    }

    // get next piece
    piece = std::strtok(nullptr, ",");
  }
}

} // namespace seqgen

#endif // CPPLIB_SEQUENCEGEN_HPP
