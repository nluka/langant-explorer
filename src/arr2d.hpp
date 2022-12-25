#ifndef CPPLIB_ARR2D_HPP
#define CPPLIB_ARR2D_HPP

#include <cstdlib>

// Collection of pure functions for working with contiguously allocated 2D arrays.
namespace arr2d {

// Returns a 1-dimensional index into a 2-dimensional array.
// `targetCol` and `targetRow` are zero-indexed, meaning (0, 0) is the first element.
constexpr
size_t get_1d_idx(
  size_t const arrWidth,
  size_t const targetCol,
  size_t const targetRow
) {
  return (targetRow * arrWidth) + targetCol;
}

// Returns the largest value.
template <typename ElemT>
constexpr
ElemT max(
  ElemT const *const arr,
  size_t const width,
  size_t const height,
  size_t const startIdx = 0
) {
  ElemT const *max = &arr[startIdx];
  size_t const len = width * height;
  for (size_t i = startIdx + 1; i < len; ++i) {
    if (arr[i] > *max) {
      max = &arr[i];
    }
  }
  return *max;
}

// Returns true if elements (beginning from `startIdx`) between the two arrays
// are the same, false otherwise.
template <typename ElemT>
constexpr
bool cmp(
  ElemT const *const arr1,
  ElemT const *const arr2,
  size_t const width,
  size_t const height,
  size_t const startIdx = 0
) {
  size_t const len = width * height;
  for (size_t i = startIdx; i < len; ++i) {
    if (arr1[i] != arr2[i]) {
      return false;
    }
  }
  return true;
}

// Returns true if all array elements beginning from `startIdx` are the same,
// false otherwise.
template <typename ElemT>
constexpr
bool is_homogenous(
  ElemT const *const arr,
  size_t const width,
  size_t const height,
  size_t const startIdx = 0
) {
  size_t const len = width * height;
  ElemT const &firstElem = arr[startIdx];
  for (size_t i = startIdx + 1; i < len; ++i) {
    if (arr[i] != firstElem) {
      return false;
    }
  }
  return true;
}

} // namespace arr2d

#endif // CPPLIB_ARR2D_HPP
