#include "simpletorrent/Tracker.h"

#include "simpletorrent/HttpTracker.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/UdpTracker.h"

namespace simpletorrent {
Tracker::Tracker(const std::vector<std::string>& tracker_url_list,
                 const std::string& info_hash, const std::string& our_id)
    : tracker_url_list_(tracker_url_list),
      info_hash_(info_hash),
      our_id_(our_id) {}

void Tracker::add_peers() {
  int prv_num = peer_set_.size();

  // add peers from udp trackers
  asio::io_context io_context;
  std::vector<UdpTracker> udp_trackers;
  for (const auto& url : tracker_url_list_) {
    if (url.starts_with("udp")) {
      UdpTracker ut(url, info_hash_, our_id_, peer_set_, io_context);
      udp_trackers.push_back(std::move(ut));
    }
  }
  for (auto& udp_tracker : udp_trackers) {
    udp_tracker.add_peers();
  }
  io_context.run();

  LOG_INFO("Num peers added by udp trackers: {}", peer_set_.size() - prv_num);

  if (peer_set_.size() > 0) {
    LOG_INFO("Skipping HTTP trackers");
    return;
  }

  prv_num = peer_set_.size();

  // add peers from http trackers
  for (const auto& url : tracker_url_list_) {
    if (url.starts_with("http")) {
      HttpTracker ht(url, info_hash_, our_id_, peer_set_);
      ht.add_peers();
    }
  }

  LOG_INFO("Num peers added by HTTP trackers: {}", peer_set_.size() - prv_num);
  LOG_INFO("Finished adding peers from trackers");
}

std::vector<PeerConnInfo> Tracker::get_peers() const {
  std::vector<PeerConnInfo> res;
  for (const auto& s : peer_set_) {
    auto [ip, port] = split_conn_string(s);
    res.emplace_back(ip, port);
  }
  return res;
}

std::pair<std::string, uint16_t> Tracker::split_conn_string(
    const std::string& conn_string) const {
  std::size_t colon_pos = conn_string.find(':');

  std::string ip = conn_string.substr(0, colon_pos);
  std::string port_str = conn_string.substr(colon_pos + 1);

  int int_port = std::stoi(port_str);
  if (int_port < 0 || int_port > UINT16_MAX) {
    throw std::out_of_range("port out of range");
  }
  uint16_t port = static_cast<uint16_t>(int_port);

  return {ip, port};
}
}  // namespace simpletorrent