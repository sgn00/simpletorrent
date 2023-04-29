#pragma once
#include <bencode.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace simpletorrent {

struct FileMetadata {
  long long file_length;
  std::vector<std::string> paths;
};

struct TorrentMetadata {
  std::string announce_url;
  std::vector<std::string> piece_hashes;
  long long piece_length;
  long long total_length;
  std::string output_path;
  std::string info_hash;
  std::vector<FileMetadata> files;
};

class Parser {
 public:
  TorrentMetadata parse_torrent_file(const std::string& torrent_file);

 private:
  void parse_single_file_info(const bencode::dict& info_dict);

  void parse_multi_file_info(const bencode::dict& info_dict);

  std::vector<std::string> parse_piece_hash(const bencode::dict& info_dict);

  std::string calc_info_hash(const bencode::dict& data_dict);
};

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const std::string& message)
      : std::runtime_error(message) {}
};

}  // namespace simpletorrent