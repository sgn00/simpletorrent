#include "simpletorrent/Peer.h"

#include <iostream>

using namespace std::literals;

namespace simpletorrent {

Peer::Peer(PieceManager& piece_manager, asio::io_context& io_context,
           const std::string& info_hash, const std::string& our_id,
           const std::string& ip_address, unsigned short port)
    : piece_manager_(piece_manager),
      socket_(io_context),
      info_hash_(info_hash),
      our_id_(our_id),
      ip_address_(ip_address),
      port_(port) {}

asio::awaitable<void> Peer::start() {
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip_address_), port_);

  auto [ec] = co_await socket_.async_connect(
      endpoint, asio::as_tuple(asio::use_awaitable));

  if (ec) {
    std::cout << "error connecting: " << ec << std::endl;
    co_return;
  } else {
    std::cout << "Connected!" << std::endl;
  }

  ec = co_await send_handshake();

  if (ec) {
    std::cout << "error sending handshake: " << ec << std::endl;
    co_return;
  }

  ec = co_await receive_handshake_response();

  if (ec) {
    std::cout << "error receiving handhsake: " << ec << std::endl;
    co_return;
  }

  // co_await async_connect(socket_, endpoints, asio::use_awaitable);
  // co_await send_handshake();
  // co_await read_handshake_response();
}

asio::awaitable<std::error_code> Peer::send_handshake() {
  static constexpr auto protocol_identifier = "BitTorrent protocol"sv;
  static constexpr auto reserved_bytes = std::array<uint8_t, 8>{};

  std::vector<uint8_t> handshake;

  // 1. Length of the protocol identifier
  handshake.push_back(static_cast<uint8_t>(protocol_identifier.size()));

  // 2. Protocol identifier
  handshake.insert(handshake.end(), protocol_identifier.begin(),
                   protocol_identifier.end());

  // 3. 8 reserved bytes
  handshake.insert(handshake.end(), reserved_bytes.begin(),
                   reserved_bytes.end());

  // 4. Info_hash
  handshake.insert(handshake.end(), info_hash_.begin(), info_hash_.end());

  // 5. Peer_id
  handshake.insert(handshake.end(), our_id_.begin(), our_id_.end());

  auto [ec, n] = co_await asio::async_write(
      socket_, asio::buffer(handshake), asio::as_tuple(asio::use_awaitable));
  std::cout << "sent handshake" << std::endl;
  co_return ec;
}

asio::awaitable<std::error_code> Peer::receive_handshake_response() {
  std::array<uint8_t, 68> handshake_response;
  auto [ec, n] =
      co_await asio::async_read(socket_, asio::buffer(handshake_response),
                                asio::as_tuple(asio::use_awaitable));

  if (ec) {
    co_return ec;
  }
  co_return ec;

  // bool parsed_success = parse_handshake_response(handshake_response);
  // if (parsed_success) {
  //   co_return std::error_code{};
  // } else {
  //   co_return std::error_code{};
  // }
}

asio::awaitable<void> Peer::receive_messages() {
  // while (!piece_manager_.is_download_complete()) {
  // }
  co_return;
}
}  // namespace simpletorrent