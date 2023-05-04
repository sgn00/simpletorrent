#pragma once
#include <bencode.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "Metadata.h"

namespace simpletorrent {

namespace parser {

TorrentMetadata parse_torrent_file(const std::string& torrent_file);

namespace {
void parse_multi_file_info(const bencode::dict& info_dict);

std::vector<std::string> parse_piece_hash(const bencode::dict& info_dict);

std::string calc_info_hash(const bencode::dict& data_dict);

void add_trackers(const bencode::dict& torrent_data_dict,
                  TorrentMetadata& data);

void add_files(const bencode::dict& info_dict, TorrentMetadata& data);
}  // namespace

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const std::string& message)
      : std::runtime_error(message) {}
};
}  // namespace parser

}  // namespace simpletorrent