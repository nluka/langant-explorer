#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

static_assert(sizeof(int8_t) == 1);
typedef int8_t i8;

static_assert(sizeof(int16_t) == 2);
typedef int16_t i16;

static_assert(sizeof(int32_t) == 4);
typedef int32_t i32;

static_assert(sizeof(int64_t) == 8);
typedef int64_t i64;

static_assert(sizeof(uint8_t) == 1);
typedef uint8_t u8;

static_assert(sizeof(uint16_t) == 2);
typedef uint16_t u16;

static_assert(sizeof(uint32_t) == 4);
typedef uint32_t u32;

static_assert(sizeof(uint64_t) == 8);
typedef uint64_t u64;

typedef size_t usize;

typedef float f32;
static_assert(sizeof(float) == 4);

static_assert(sizeof(double) == 8);
typedef double f64;

#endif // TYPES_HPP