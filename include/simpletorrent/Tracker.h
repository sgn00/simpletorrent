#pragma once
#include <asio.hpp>
#include <unordered_set>
#include <vector>

#include "Metadata.h"

namespace simpletorrent {
class Tracker {
 public:
  Tracker(const std::vector<std::string>& tracker_url_list,
          const std::string& info_hash, const std::string& our_id);

  void add_peers();

  std::vector<PeerConnInfo> get_peers() const;

 private:
  std::vector<std::string> tracker_url_list_;
  std::string info_hash_;
  std::string our_id_;
  std::unordered_set<std::string> peer_set_;

  std::pair<std::string, uint16_t> split_conn_string(
      const std::string& conn_string) const;
};
}  // namespace simpletorrent