#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

int main() {
  std::string torrent_file = "../debian-iso.torrent";

  // Open the torrent file for reading
  std::ifstream file(torrent_file, std::ios::binary);
  if (!file) {
    std::cerr << "Error: failed to open file " << torrent_file << std::endl;
    return 1;
  }

  // Parse the torrent file using bencode::decode
  auto torrent_data = bencode::decode(file);
  auto value = std::get<bencode::dict>(torrent_data);
  auto a = std::get<bencode::string>(value.at("announce"));
  std::cout << a << std::endl;
  // Extract data from the torrent_data object as needed
  // std::cout << str << std::endl;

  return 0;
}