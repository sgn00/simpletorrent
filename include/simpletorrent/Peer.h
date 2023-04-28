
#pragma once

#include <asio.hpp>

#include "MessageUtil.h"
#include "PieceManager.h"

using namespace std::literals;

namespace simpletorrent {

class Peer {
 public:
  Peer(PieceManager& piece_manager, asio::io_context& io_context,
       const std::string& info_hash, const std::string& our_id,
       const std::string& ip_address, uint16_t port, uint32_t peer_num_id,
       uint32_t num_pieces);

  Peer(const Peer&) = delete;
  Peer& operator=(const Peer&) = delete;

  asio::awaitable<void> start();

  uint32_t get_id() const;

  bool exited_;

 private:
  PieceManager& piece_manager_;
  std::string info_hash_;
  std::string our_id_;
  std::string ip_address_;
  uint16_t port_;
  std::string peer_id_;
  uint8_t num_in_flight_;
  uint32_t peer_num_id_;
  bool is_choked_ = true;
  asio::io_context& io_context_;
  uint32_t num_pieces_;
  bool continue_connection_;
  asio::ip::tcp::socket socket_;

  static constexpr int MAX_IN_FLIGHT = 5;

  asio::awaitable<void> send_handshake();

  asio::awaitable<void> receive_handshake_response();

  asio::awaitable<void> send_interested();

  asio::awaitable<void> receive_messages();

  asio::awaitable<void> send_messages();

  asio::awaitable<void> send_block_requests();

  void handle_bitfield_message(const std::vector<uint8_t>& payload);

  bool handle_piece_message(const std::vector<uint8_t>& payload);

  void set_timeout(uint32_t timeout_seconds, asio::steady_timer& timer);

  asio::awaitable<uint32_t> read_header(asio::steady_timer& timer,
                                        std::vector<uint8_t>& header);

  void handle_message(message_util::MessageType type,
                      const std::vector<uint8_t>& payload);
};

}  // namespace simpletorrent