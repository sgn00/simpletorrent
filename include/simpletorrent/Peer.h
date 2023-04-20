
#pragma once

#include <asio.hpp>

#include "simpletorrent/PieceManager.h"

using namespace std::literals;

namespace simpletorrent {

static constexpr auto protocol_identifier = "BitTorrent protocol"sv;

class Peer {
 public:
  Peer(PieceManager& piece_manager, asio::io_context& io_context,
       const std::string& info_hash, const std::string& our_id,
       const std::string& ip_address, uint16_t port, uint32_t peer_num_id);

  asio::awaitable<void> start();

 private:
  PieceManager& piece_manager_;
  asio::ip::tcp::socket socket_;
  std::string info_hash_;
  std::string our_id_;
  std::string ip_address_;
  uint16_t port_;
  std::string peer_id_;
  uint8_t num_in_flight_;
  uint32_t peer_num_id_;
  bool is_choked_ = true;
  asio::io_context& io_context_;

  static constexpr int MAX_IN_FLIGHT = 5;

  asio::awaitable<std::error_code> send_handshake();

  asio::awaitable<std::error_code> receive_handshake_response();

  asio::awaitable<std::error_code> send_interested();

  asio::awaitable<std::error_code> receive_messages();

  asio::awaitable<void> send_messages();

  asio::awaitable<std::error_code> send_block_requests();

  bool parse_handshake_response(
      const std::array<uint8_t, 68>& handshake_response);

  void handle_bitfield_message(const std::vector<uint8_t>& payload);

  bool handle_piece_message(const std::vector<uint8_t>& payload);
};
}  // namespace simpletorrent