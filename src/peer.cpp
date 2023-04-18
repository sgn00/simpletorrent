#include "simpletorrent/Peer.h"

#include <iostream>

#include "simpletorrent/MessageUtil.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

Peer::Peer(PieceManager& piece_manager, asio::io_context& io_context,
           const std::string& info_hash, const std::string& our_id,
           const std::string& ip_address, uint32_t port)
    : piece_manager_(piece_manager),
      socket_(io_context),
      info_hash_(info_hash),
      our_id_(our_id),
      ip_address_(ip_address),
      port_(port),
      num_in_flight_(0) {}

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
    std::cout << "error receiving handshake: " << ec << std::endl;
    co_return;
  }

  ec = co_await send_interested();
  if (ec) {
    std::cout << "error during send interested" << std::endl;
    co_return;
  }

  // co_await async_connect(socket_, endpoints, asio::use_awaitable);
  // co_await send_handshake();
  // co_await read_handshake_response();
}

asio::awaitable<std::error_code> Peer::send_handshake() {
  static constexpr auto reserved_bytes = std::array<uint8_t, 8>{};

  std::vector<uint8_t> handshake;

  std::string binary_info_hash = hex_decode(info_hash_);
  std::string binary_our_id = hex_decode(our_id_);

  // 1. Length of the protocol identifier
  handshake.push_back(static_cast<uint8_t>(protocol_identifier.size()));

  // 2. Protocol identifier
  handshake.insert(handshake.end(), protocol_identifier.begin(),
                   protocol_identifier.end());

  // 3. 8 reserved bytes
  handshake.insert(handshake.end(), reserved_bytes.begin(),
                   reserved_bytes.end());

  // 4. Info_hash
  handshake.insert(handshake.end(), binary_info_hash.begin(),
                   binary_info_hash.end());

  // 5. Peer_id
  handshake.insert(handshake.end(), binary_our_id.begin(), binary_our_id.end());

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

  bool parsed_success = parse_handshake_response(handshake_response);
  if (parsed_success) {
    co_return std::error_code{};
  } else {
    co_return std::error_code{};
  }
}

bool Peer::parse_handshake_response(
    const std::array<uint8_t, 68>& handshake_response) {
  size_t idx = 0;

  // Extract the protocol name length (first byte)
  uint8_t protocol_name_length = handshake_response[idx++];

  // Extract the protocol name
  std::string protocol_name(
      handshake_response.begin() + idx,
      handshake_response.begin() + idx + protocol_name_length);
  idx += protocol_name_length;

  if (protocol_name != protocol_identifier) {
    return false;
  }

  // Extract the reserved bytes
  std::array<uint8_t, 8> reserved_bytes;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 8, reserved_bytes.begin());
  idx += 8;

  // Extract the info_hash
  std::array<uint8_t, 20> binary_info_hash;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 20, binary_info_hash.begin());
  idx += 20;

  std::string hex_info_hash = to_hex_string(binary_info_hash);

  if (hex_info_hash != info_hash_) {
    return false;
  }

  // Extract the peer_id
  std::array<uint8_t, 20> binary_peer_id;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 20, binary_peer_id.begin());
  peer_id_ = to_hex_string(binary_peer_id);

  return true;
}

asio::awaitable<std::error_code> Peer::send_interested() {
  auto interested_message =
      message_util::construct_message(message_util::MessageType::Interested);
  auto [ec, n] =
      co_await asio::async_write(socket_, asio::buffer(interested_message),
                                 asio::as_tuple(asio::use_awaitable));
  std::cout << "Sent interested message" << std::endl;
  co_return ec;
}

asio::awaitable<std::error_code> Peer::receive_messages() {
  using namespace message_util;
  std::vector<uint8_t> header(4);

  // Keep attempting to read messages until download is complete
  while (!piece_manager_.is_download_complete()) {
    auto [ec, n] = co_await asio::async_read(
        socket_, asio::buffer(header), asio::as_tuple(asio::use_awaitable));
    if (ec) {
      co_return ec;
    }

    uint32_t message_length = get_header_length(header);

    // Handle keep-alive message
    if (message_length == 0) {
      std::cout << "Received keep-alive message" << std::endl;
      continue;
    }

    // Read message type
    uint8_t message_type;
    auto [ec2, n2] =
        co_await asio::async_read(socket_, asio::buffer(&message_type, 1),
                                  asio::as_tuple(asio::use_awaitable));
    if (ec2) {
      co_return ec;
    }

    MessageType type = static_cast<MessageType>(message_type);

    uint32_t payload_length = message_length - 1;

    // Read payload
    std::vector<uint8_t> payload(payload_length);
    auto [ec3, n3] = co_await asio::async_read(
        socket_, asio::buffer(payload), asio::as_tuple(asio::use_awaitable));
    if (ec3) {
      co_return ec3;
    }

    // Handle different message types
    switch (type) {
      case MessageType::Choke:
        std::cout << "Received choke message" << std::endl;
        is_choked_ = false;
        break;
      case MessageType::Unchoke:
        std::cout << "Received unchoke message" << std::endl;
        // Handle unchoke message
        break;
      case MessageType::Interested:
        std::cout << "Received interested message" << std::endl;
        // Handle interested message
        break;
      case MessageType::NotInterested:
        std::cout << "Received not interested message" << std::endl;
        // Handle not interested message
        break;
      case MessageType::Have:
        std::cout << "Received have message" << std::endl;
        // Handle have message
        break;
      case MessageType::Bitfield:
        std::cout << "Received bitfield message" << std::endl;
        handle_bitfield_message(payload);
        // Handle bitfield message
        break;
      case MessageType::Request:
        std::cout << "Received request message" << std::endl;
        // Handle request message
        break;
      case MessageType::Piece:
        std::cout << "Received piece message" << std::endl;

        // Handle piece message
        break;
      case MessageType::Cancel:
        std::cout << "Received cancel message" << std::endl;
        // Handle cancel message
        break;
      case MessageType::Port:
        std::cout << "Received port message" << std::endl;
        // Handle port message
        break;
      default:
        std::cout << "error" << std::endl;
    }

    auto ec4 = co_await send_block_requests();
    if (ec4) {
      co_return ec4;
    }
  }
  co_return std::error_code{};
}

void Peer::handle_bitfield_message(const std::vector<uint8_t>& payload) {
  // Calculate the number of pieces based on the payload length
  size_t payload_length = payload.size();
  uint32_t num_pieces = payload_length * 8;

  // Initialize a vector to represent the availability of pieces
  std::vector<uint8_t> peer_bitfield(num_pieces);

  // Parse the bitfield payload
  for (uint32_t i = 0; i < payload_length; ++i) {
    for (uint8_t j = 0; j < 8; ++j) {
      bool has_piece = (payload[i] & (0x80 >> j)) != 0;
      uint32_t piece_index = i * 8 + j;

      if (piece_index < num_pieces) {
        peer_bitfield[piece_index] = has_piece;
      }
    }
  }

  // Handle the parsed bitfield data (e.g., store it, update the state, or start
  // requesting pieces)
  piece_manager_.update_piece_frequencies(peer_bitfield, 0);
}

asio::awaitable<std::error_code> Peer::send_block_requests() {
  // keep attempting to send messages
  while (num_in_flight_ < MAX_IN_FLIGHT) {
    auto block_request = piece_manager_.select_next_block(0);
    if (!block_request.has_value()) {
      break;
    }
    auto request_message =
        message_util::construct_request_message(block_request.value());
    auto [ec, n] =
        co_await asio::async_write(socket_, asio::buffer(request_message),
                                   asio::as_tuple(asio::use_awaitable));
    if (ec) {
      co_return ec;
    }
    num_in_flight_++;
  }
  co_return std::error_code{};
}

bool Peer::handle_piece_message(const std::vector<uint8_t>& payload) {
  if (payload.size() < 9) {
    std::cerr << "Malformed piece message (too short)" << std::endl;
    return false;
  }

  auto [piece_index, block_offset] =
      message_util::retrieve_piece_index_and_block_offset(payload);

  // Extract the block data
  Block block{piece_index, block_offset, payload.begin() + 8, payload.end()};
  piece_manager_.add_block(0, block);

  num_in_flight_--;
  return true;
}

}  // namespace simpletorrent