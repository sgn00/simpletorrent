#pragma once

#include <bencode.hpp>
#include <optional>
#include <string>
#include <unordered_set>

#include "Metadata.h"

namespace simpletorrent {

const int PORT = 8080;

const int TRACKER_TIMEOUT = 15000;

class Tracker {
 public:
  Tracker(const std::vector<std::string>& tracker_url_list,
          const std::string& info_hash, const std::string& our_id);
  std::vector<PeerConnInfo> get_peers();

 private:
  void get_peers_from_tracker(const std::string& tracker_url);

  std::optional<bencode::data> send_request(
      const std::string& tracker_url) const;
  void parse_tracker_response(const bencode::data& response);

  std::pair<std::string, uint16_t> split_conn_string(
      const std::string& conn_string) const;

  std::vector<std::string> tracker_url_list_;
  std::string info_hash_;
  std::string our_id_;
  std::unordered_set<std::string> peer_conn_str_set_;
};

}  // namespace simpletorrent