#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

#include "simpletorrent/Logger.h"

namespace simpletorrent {

inline std::string hex_decode(const std::string& value) {
  std::string decodedHexString;
  decodedHexString.reserve(value.size() / 2);

  for (size_t i = 0; i < value.size(); i += 2) {
    uint8_t decoded_byte;
    std::from_chars(&value[i], &value[i] + 2, decoded_byte, 16);
    decodedHexString.push_back(static_cast<char>(decoded_byte));
  }

  return decodedHexString;
}

template <size_t SIZE>
inline std::string to_hex_string(const std::array<uint8_t, SIZE>& arr) {
  std::stringstream ss;
  for (const auto& byte : arr) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte);
  }
  return ss.str();
}

// Define a concept that checks if the type is uint16_t, uint32_t, uint64_t
template <typename T>
concept Uint = std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t> ||
               std::is_same_v<T, uint16_t>;

template <Uint T>
T convert_bo(T value) {
  if constexpr (std::endian::native == std::endian::little) {
    T result = value;
    auto first = reinterpret_cast<char*>(&result);
    auto last = first + sizeof(T);
    std::reverse(first, last);
    return result;
  } else {
    return value;
  }
}

}  // namespace simpletorrent