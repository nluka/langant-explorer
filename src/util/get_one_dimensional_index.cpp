#include "util.hpp"

size_t get_one_dimensional_index(
  size_t const arrWidth,
  size_t const targetColumn,
  size_t const targetRow
) {
  return (targetRow * arrWidth) + targetColumn;
}
