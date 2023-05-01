#pragma once
#include <bencode.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "Metadata.h"

namespace simpletorrent {

class Parser {
 public:
  TorrentMetadata parse_torrent_file(const std::string& torrent_file) const;

 private:
  void parse_multi_file_info(const bencode::dict& info_dict) const;

  std::vector<std::string> parse_piece_hash(
      const bencode::dict& info_dict) const;

  std::string calc_info_hash(const bencode::dict& data_dict) const;

  void add_trackers(const bencode::dict& torrent_data_dict,
                    TorrentMetadata& data) const;

  void add_files(const bencode::dict& info_dict, TorrentMetadata& data) const;
};

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const std::string& message)
      : std::runtime_error(message) {}
};

}  // namespace simpletorrent