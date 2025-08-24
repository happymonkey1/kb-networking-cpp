//
// Created by happymonkey1 on 8/16/25.
//

#ifndef KB_NETWORKING_CPP_TYPES_H
#define KB_NETWORKING_CPP_TYPES_H

#include <cstdint>
#include <optional>

namespace kb {

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

template <typename T>
using option = std::optional<T>;

}

#endif  //KB_NETWORKING_CPP_TYPES_H
