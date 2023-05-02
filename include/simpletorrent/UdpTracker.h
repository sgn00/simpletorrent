#pragma once
#include <asio.hpp>
#include <cstdint>

#include "Metadata.h"

namespace simpletorrent {
class UdpTracker {
 public:
  UdpTracker(const std::string& tracker_url, const std::string& info_hash,
             const std::string& our_id);

  std::vector<std::string> get_peers();

  void connect();

 private:
  asio::io_context io_context_;
  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint tracker_endpoint_;

  std::string info_hash_;
  std::string our_id_;

  asio::awaitable<uint64_t> send_connect_request();

  asio::awaitable<void> send_announce_request(uint64_t connection_id,
                                              std::vector<std::string>& res);
};
}  // namespace simpletorrent