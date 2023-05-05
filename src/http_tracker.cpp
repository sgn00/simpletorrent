#include <cpr/cpr.h>

#include <asio.hpp>

#include "simpletorrent/HttpTracker.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

HttpTracker::HttpTracker(const std::string& tracker_url,
                         const std::string& info_hash,
                         const std::string& our_id,
                         std::unordered_set<std::string>& peer_set)
    : tracker_url_(tracker_url),
      info_hash_(info_hash),
      our_id_(our_id),
      peer_set_(peer_set) {}

void HttpTracker::add_peers() {
  LOG_INFO("Connecting to tracker at {}", tracker_url_);
  auto response = send_request(tracker_url_);
  if (!response) {
    LOG_CRITICAL("Error getting response from tracker at {}", tracker_url_);
    return;
  }
  try {
    parse_tracker_response(response.value());
  } catch (const std::exception& e) {
    LOG_CRITICAL("Error parsing tracker response at {} | Error: {}",
                 tracker_url_, e.what());
  }
}

std::optional<bencode::data> HttpTracker::send_request(
    const std::string& tracker_url) const {
  auto parameters = cpr::Parameters{
      {"info_hash", hex_decode(info_hash_)},
      {"peer_id", our_id_},
      {"port", std::to_string(PORT)},
      {"compact", "1"},
      {"numwant",
       std::string(
           MAX_PEERS_FROM_TRACKER)}  // Request up to 200 peers from the tracker
  };
  cpr::Response response = cpr::Get(cpr::Url{tracker_url}, parameters,
                                    cpr::Timeout{TRACKER_TIMEOUT});
  if (response.status_code != 200) {
    return std::nullopt;
  }

  return bencode::decode(response.text);
}

void HttpTracker::parse_tracker_response(const bencode::data& response) {
  auto response_dict = std::get<bencode::dict>(response);

  int prv_num = peer_set_.size();

  auto peer_data = response_dict.at("peers");
  if (std::holds_alternative<bencode::list>(
          peer_data)) {  // peers can be in list form
    auto peer_list = std::get<bencode::list>(peer_data);
    for (auto& peer : peer_list) {
      auto peer_map = std::get<bencode::dict>(peer);
      auto ip = std::get<bencode::string>(peer_map.at("ip"));
      auto port = std::get<bencode::integer>(peer_map.at("port"));
      peer_set_.insert(ip + ":" + std::to_string(port));
    }
  } else if (std::holds_alternative<bencode::string>(
                 peer_data)) {  // or string form
    auto peers_string = std::get<bencode::string>(peer_data);
    for (size_t i = 0; i < peers_string.size(); i += 6) {
      asio::ip::address_v4::bytes_type ip_bytes;
      std::memcpy(ip_bytes.data(), peers_string.data() + i, 4);
      uint16_t port = convert_bo(
          *(reinterpret_cast<std::uint16_t*>(peers_string.data() + i + 4)));
      asio::ip::address_v4 ip_address(ip_bytes);
      std::string conn_str =
          ip_address.to_string() + ":" + std::to_string(port);
      peer_set_.insert(conn_str);
    }
  } else {
    LOG_ERROR("Invalid peer data from tracker at {}", tracker_url_);
    return;
  }

  LOG_INFO("HTTP tracker at {} contributed {} peers", tracker_url_,
           peer_set_.size() - prv_num);
}

}  // namespace simpletorrent