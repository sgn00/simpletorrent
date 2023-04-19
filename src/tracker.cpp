#include "simpletorrent/Tracker.h"

#include <arpa/inet.h>
#include <cpr/cpr.h>

#include "simpletorrent/Util.h"

namespace simpletorrent {

Tracker::Tracker(const std::string& announce_url, const std::string& info_hash,
                 const std::string& our_id)
    : announce_url_(announce_url), info_hash_(info_hash), our_id_(our_id) {}

const std::vector<PeerConnInfo>& Tracker::get_peers() const { return peers_; }

bool Tracker::update_peers() {
  auto response = send_request();
  if (!response) {
    return false;
  }
  parse_tracker_response(response.value());
  return true;
}

std::optional<bencode::data> Tracker::send_request() const {
  auto parameters = cpr::Parameters{
      {"info_hash", hex_decode(info_hash_)},
      {"peer_id", our_id_},
      {"port", std::to_string(PORT)},
      {"compact", "1"},
      {"numwant", "50"}  // Request up to 50 peers from the tracker
  };
  cpr::Response response = cpr::Get(cpr::Url{announce_url_}, parameters,
                                    cpr::Timeout{TRACKER_TIMEOUT});
  if (response.status_code != 200) {
    std::cerr << "Failed to get 200 response from Tracker: "
              << response.error.message << std::endl;
    return std::nullopt;
  }

  return bencode::decode(response.text);
}

void Tracker::parse_tracker_response(const bencode::data& response) {
  auto response_dict = std::get<bencode::dict>(response);
  auto peers_string = std::get<bencode::string>(response_dict.at("peers"));

  peers_.clear();
  for (size_t i = 0; i < peers_string.size(); i += 6) {
    char ip_str[INET_ADDRSTRLEN];
    uint32_t ip = *reinterpret_cast<const uint32_t*>(peers_string.data() + i);
    uint16_t port =
        ntohs(*reinterpret_cast<const uint16_t*>(peers_string.data() + i + 4));

    inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
    peers_.push_back({ip_str, port});
    // std::ostringstream peer;
    // peer << ip_str << ':' << port;
    // peers_.push_back(peer.str());
  }
}

}  // namespace simpletorrent