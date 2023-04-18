#pragma once

#include <array>
#include <iomanip>
#include <sstream>
#include <string>

namespace simpletorrent {

// Generated by ChatGPT
inline std::string hex_decode(const std::string& hex_string) {
  std::string binary_string;

  for (std::size_t i = 0; i < hex_string.length(); i += 2) {
    std::string hex_pair = hex_string.substr(i, 2);

    char byte = static_cast<char>(std::stoi(hex_pair, nullptr, 16));

    binary_string.push_back(byte);
  }
  return binary_string;
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

}  // namespace simpletorrent