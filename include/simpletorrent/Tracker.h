#pragma once

#include <bencode.hpp>
#include <optional>
#include <string>

#include "Metadata.h"

namespace simpletorrent {

const int PORT = 8080;

const int TRACKER_TIMEOUT = 15000;

class Tracker {
 public:
  Tracker(const std::string& announce_url, const std::string& info_hash,
          const std::string& our_id);
  const std::vector<PeerConnInfo>& get_peers() const;
  const std::string& get_info_hash() const;
  void update_peers();

 private:
  std::optional<bencode::data> send_request() const;
  void parse_tracker_response(const bencode::data& response);

  std::string announce_url_;
  std::string info_hash_;
  std::string our_id_;
  std::vector<PeerConnInfo> peers_;
};

}  // namespace simpletorrent