
#pragma once

#include <asio.hpp>

#include "simpletorrent/PieceManager.h"

namespace simpletorrent {
class Peer {
 public:
  Peer(PieceManager& piece_manager, asio::io_context& io_context,
       const std::string& info_hash, const std::string& our_id,
       const std::string& ip_address, unsigned short port);

  asio::awaitable<void> start();

 private:
  PieceManager& piece_manager_;
  asio::ip::tcp::socket socket_;
  std::string info_hash_;
  std::string our_id_;
  std::string ip_address_;
  unsigned short port_;

  asio::awaitable<std::error_code> send_handshake();

  asio::awaitable<std::error_code> receive_handshake_response();

  asio::awaitable<void> send_interested();

  asio::awaitable<void> receive_messages();

  asio::awaitable<void> send_block_request();

  bool parse_handshake_response(
      const std::array<uint8_t, 68>& handshake_response);
};
}  // namespace simpletorrent