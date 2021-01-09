//
// Created by TYTY on 2020-12-31 031.
//

#ifndef BBCODE__DEFS_H_
#define BBCODE__DEFS_H_

#include <cstdint>
#include <type_traits>

typedef int8_t i8;
typedef uint8_t u8;

typedef int16_t i16;
typedef uint16_t u16;

typedef int32_t i32;
typedef uint32_t u32;

typedef int64_t i64;
typedef uint64_t u64;

typedef std::size_t usize;
typedef std::make_signed<std::size_t>::type isize;

typedef float f32;
typedef double f64;
typedef long double f80;

#endif //BBCODE__DEFS_H_
