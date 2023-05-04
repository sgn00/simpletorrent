#pragma once
#include <asio.hpp>
#include <cstdint>
#include <unordered_set>

#include "Metadata.h"

namespace simpletorrent {
class UdpTracker {
 public:
  UdpTracker(const std::string& tracker_url, const std::string& info_hash,
             const std::string& our_id,
             std::unordered_set<std::string>& peer_set,
             asio::io_context& io_context);

  void add_peers();

  std::string get_url() const;

 private:
  std::string tracker_url_;
  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint tracker_endpoint_;

  std::string info_hash_;
  std::string our_id_;
  std::unordered_set<std::string>& peer_set_;

  asio::io_context& io_context_;

  asio::awaitable<uint64_t> send_connect_request();

  asio::awaitable<void> send_announce_request(uint64_t connection_id);

  static std::string extract_host(const std::string& announce_url);

  static std::string extract_port(const std::string& announce_url);

  static uint32_t random_uint32();
};
}  // namespace simpletorrent