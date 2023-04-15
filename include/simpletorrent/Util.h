#pragma once

#include <string>

namespace simpletorrent {

// Generated by ChatGPT
std::string hex_decode(const std::string& hex_string) {
  std::string binary_string;

  for (std::size_t i = 0; i < hex_string.length(); i += 2) {
    std::string hex_pair = hex_string.substr(i, 2);

    char byte = static_cast<char>(std::stoi(hex_pair, nullptr, 16));

    binary_string.push_back(byte);
  }
  return binary_string;
}

}  // namespace simpletorrent