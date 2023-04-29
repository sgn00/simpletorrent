#include "simpletorrent/Tracker.h"

#include <arpa/inet.h>
#include <cpr/cpr.h>

#include "simpletorrent/Logger.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

Tracker::Tracker(const std::string& announce_url, const std::string& info_hash,
                 const std::string& our_id)
    : announce_url_(announce_url), info_hash_(info_hash), our_id_(our_id) {}

const std::vector<PeerConnInfo>& Tracker::get_peers() const { return peers_; }

void Tracker::update_peers() {
  LOG_INFO("Connecting to tracker at {}", announce_url_);
  auto response = send_request();
  if (!response) {
    LOG_CRITICAL("Error getting response from tracker at {}", announce_url_);
    terminate_program();
  }
  try {
    parse_tracker_response(response.value());
  } catch (const std::exception& e) {
    LOG_CRITICAL("Error parsing tracker response at {} | Error: {}",
                 announce_url_, e.what());
    terminate_program();
  }
}

std::optional<bencode::data> Tracker::send_request() const {
  auto parameters = cpr::Parameters{
      {"info_hash", hex_decode(info_hash_)},
      {"peer_id", our_id_},
      {"port", std::to_string(PORT)},
      {"compact", "1"},
      {"numwant", "200"}  // Request up to 200 peers from the tracker
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
  std::cout << "got response_dict" << std::endl;

  auto peer_data = response_dict.at("peers");
  if (std::holds_alternative<bencode::list>(peer_data)) {
    auto peer_list = std::get<bencode::list>(peer_data);
    peers_.clear();
    for (auto& peer : peer_list) {
      auto peer_map = std::get<bencode::dict>(peer);
      auto ip = std::get<bencode::string>(peer_map.at("ip"));
      auto port = std::get<bencode::integer>(peer_map.at("port"));
      std::cout << ip << ":" << std::to_string(port) << std::endl;
      peers_.push_back({ip, static_cast<uint16_t>(port)});
    }
  } else if (std::holds_alternative<bencode::string>(peer_data)) {
    auto peers_string = std::get<bencode::string>(peer_data);
    peers_.clear();
    for (size_t i = 0; i < peers_string.size(); i += 6) {
      char ip_str[INET_ADDRSTRLEN];
      uint32_t ip = *reinterpret_cast<const uint32_t*>(peers_string.data() + i);
      uint16_t port = ntohs(
          *reinterpret_cast<const uint16_t*>(peers_string.data() + i + 4));

      inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
      peers_.push_back({ip_str, port});
    }
  } else {
    std::cout << "invalid peer data" << std::endl;
    return;
  }
}

}  // namespace simpletorrent