#include <bencode.hpp>
#include <optional>
#include <string>

namespace simpletorrent {

const int PORT = 8080;

const int TRACKER_TIMEOUT = 15000;

class Tracker {
 public:
  Tracker(const std::string& announce_url, const std::string& info_hash);
  const std::vector<std::string>& get_peers() const;
  bool update_peers();

 private:
  std::optional<bencode::data> send_request() const;
  void parse_tracker_response(const bencode::data& response);

  std::string announce_url_;
  std::string info_hash_;
  std::string peer_id_;
  std::vector<std::string> peers_;
};

}  // namespace simpletorrent