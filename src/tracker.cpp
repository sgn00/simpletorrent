#include "simpletorrent/Tracker.h"

#include <arpa/inet.h>
#include <cpr/cpr.h>

#include "simpletorrent/Logger.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

Tracker::Tracker(const std::vector<std::string>& tracker_url_list,
                 const std::string& info_hash, const std::string& our_id)
    : tracker_url_list_(tracker_url_list),
      info_hash_(info_hash),
      our_id_(our_id) {
  for (auto& tracker : tracker_url_list_) {
    std::cout << tracker << std::endl;
  }
}

std::vector<PeerConnInfo> Tracker::get_peers() {
  for (const auto& tracker_url : tracker_url_list_) {
    get_peers_from_tracker(tracker_url);
  }
  std::vector<PeerConnInfo> peers;
  for (const auto& peer_str : peer_conn_str_set_) {
    auto [ip, port] = split_conn_string(peer_str);
    // std::cout << ip << ":" << std::to_string(port) << std::endl;
    peers.emplace_back(ip, port);
  }

  if (peers.size() == 0) {
    throw std::runtime_error("No peers from trackers..exiting");
  }

  return peers;
}

void Tracker::get_peers_from_tracker(const std::string& tracker_url) {
  LOG_INFO("Connecting to tracker at {}", tracker_url);
  auto response = send_request(tracker_url);
  if (!response) {
    LOG_CRITICAL("Error getting response from tracker at {}", tracker_url);
  }
  try {
    parse_tracker_response(response.value());
  } catch (const std::exception& e) {
    LOG_CRITICAL("Error parsing tracker response at {} | Error: {}",
                 tracker_url, e.what());
  }
}

std::pair<std::string, uint16_t> Tracker::split_conn_string(
    const std::string& conn_string) const {
  std::size_t colon_pos = conn_string.find(':');

  std::string ip = conn_string.substr(0, colon_pos);
  std::string port_str = conn_string.substr(colon_pos + 1);

  int int_port = std::stoi(port_str);
  if (int_port < 0 || int_port > UINT16_MAX) {
    throw std::out_of_range("Port out of range");
  }
  uint16_t port = static_cast<uint16_t>(int_port);

  return {ip, port};
}

std::optional<bencode::data> Tracker::send_request(
    const std::string& tracker_url) const {
  auto parameters = cpr::Parameters{
      {"info_hash", hex_decode(info_hash_)},
      {"peer_id", our_id_},
      {"port", std::to_string(PORT)},
      {"compact", "1"},
      {"numwant", "200"}  // Request up to 200 peers from the tracker
  };
  cpr::Response response = cpr::Get(cpr::Url{tracker_url}, parameters,
                                    cpr::Timeout{TRACKER_TIMEOUT});
  if (response.status_code != 200) {
    std::cerr << "Failed to get 200 response from Tracker " << tracker_url
              << ": " << response.error.message << std::endl;
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
    for (auto& peer : peer_list) {
      auto peer_map = std::get<bencode::dict>(peer);
      auto ip = std::get<bencode::string>(peer_map.at("ip"));
      auto port = std::get<bencode::integer>(peer_map.at("port"));
      std::cout << ip << ":" << std::to_string(port) << std::endl;
      peer_conn_str_set_.insert(ip + ":" + std::to_string(port));
    }
  } else if (std::holds_alternative<bencode::string>(peer_data)) {
    auto peers_string = std::get<bencode::string>(peer_data);
    for (size_t i = 0; i < peers_string.size(); i += 6) {
      char ip_str[INET_ADDRSTRLEN];
      uint32_t ip = *reinterpret_cast<const uint32_t*>(peers_string.data() + i);
      uint16_t port = ntohs(
          *reinterpret_cast<const uint16_t*>(peers_string.data() + i + 4));

      inet_ntop(AF_INET, &ip, ip_str, INET_ADDRSTRLEN);
      peer_conn_str_set_.insert(std::string(ip_str) + ":" +
                                std::to_string(port));
    }
  } else {
    std::cout << "invalid peer data" << std::endl;
    return;
  }
}

}  // namespace simpletorrent