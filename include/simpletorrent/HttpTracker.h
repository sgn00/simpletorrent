#pragma once

#include <bencode.hpp>
#include <optional>
#include <string>
#include <unordered_set>

#include "Metadata.h"

namespace simpletorrent {

class HttpTracker {
 public:
  HttpTracker(const std::string& tracker_url, const std::string& info_hash,
              const std::string& our_id,
              std::unordered_set<std::string>& peer_set);

  void add_peers();

 private:
  constexpr static int PORT = 8080;
  constexpr static int TRACKER_TIMEOUT = 8000;
  constexpr static std::string_view MAX_PEERS_FROM_TRACKER = "200";
  std::string tracker_url_;
  std::string info_hash_;
  std::string our_id_;
  std::unordered_set<std::string>& peer_set_;

  void get_peers_from_tracker(const std::string& tracker_url);

  std::optional<bencode::data> send_request(
      const std::string& tracker_url) const;

  void parse_tracker_response(const bencode::data& response);
};

}  // namespace simpletorrent