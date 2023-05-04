#pragma once
#include <bencode.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "Metadata.h"

namespace simpletorrent {

namespace parser {

TorrentMetadata parse_torrent_file(const std::string& torrent_file);

class ParseException : public std::runtime_error {
 public:
  explicit ParseException(const std::string& message)
      : std::runtime_error(message) {}
};
}  // namespace parser

}  // namespace simpletorrent