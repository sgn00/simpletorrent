#include "simpletorrent/Peer.h"

#include <iostream>

#include "simpletorrent/MessageUtil.h"
#include "simpletorrent/PeerException.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

Peer::Peer(PieceManager& piece_manager, asio::io_context& io_context,
           const std::string& info_hash, const std::string& our_id,
           const std::string& ip_address, uint16_t port, uint32_t peer_num_id,
           uint32_t num_pieces)
    : piece_manager_(piece_manager),
      info_hash_(info_hash),
      our_id_(our_id),
      ip_address_(ip_address),
      port_(port),
      num_in_flight_(0),
      peer_num_id_(peer_num_id),
      io_context_(io_context),
      continue_connection_(true),
      num_pieces_(num_pieces),
      socket_(io_context) {}

asio::awaitable<void> Peer::start() {
  static int peer_connected_count = 0;

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip_address_), port_);

  try {
    co_await socket_.async_connect(endpoint,
                                   asio::as_tuple(asio::use_awaitable));
  } catch (const std::exception& e) {
    std::cout << "error connecting to peer " << peer_num_id_ << ": " << e.what()
              << std::endl;
    continue_connection_ = false;
    co_return;
  }

  try {
    co_await send_handshake();
  } catch (const std::exception& e) {
    std::cout << "error sending handshake to peer " << peer_num_id_ << ": "
              << e.what() << std::endl;
    continue_connection_ = false;
    co_return;
  }

  try {
    co_await receive_handshake_response();
  } catch (const std::exception& e) {
    std::cout << "error receiving handshake from peer " << peer_num_id_ << ": "
              << e.what() << std::endl;
    continue_connection_ = false;
    co_return;
  }

  peer_connected_count++;

  try {
    co_await send_interested();
  } catch (const std::exception& e) {
    std::cout << "error sending interested to peer " << peer_num_id_ << ": "
              << e.what() << std::endl;
    peer_connected_count--;
    std::cout << "num connected peers: " << peer_connected_count << std::endl;
    continue_connection_ = false;
    co_return;
  }

  asio::co_spawn(
      io_context_, [this]() { return send_messages(); }, asio::detached);

  try {
    co_await receive_messages();
  } catch (const std::exception& e) {
    std::cout << "error during receive messages from peer " << peer_num_id_
              << ": " << e.what() << std::endl;
    continue_connection_ = false;
    peer_connected_count--;
    std::cout << "num connected peers: " << peer_connected_count << std::endl;
    continue_connection_ = false;
    co_return;
  }

  std::cout << "exiting peer gracefully " << peer_num_id_ << std::endl;
  peer_connected_count--;
  std::cout << "num connected peers: " << peer_connected_count << std::endl;

  co_return;
}

asio::awaitable<void> Peer::send_handshake() {
  static constexpr auto reserved_bytes = std::array<uint8_t, 8>{};

  std::string binary_info_hash = hex_decode(info_hash_);

  auto handshake =
      message_util::create_handshake(reserved_bytes, binary_info_hash, our_id_);

  co_await asio::async_write(socket_, asio::buffer(handshake),
                             asio::as_tuple(asio::use_awaitable));
}

asio::awaitable<void> Peer::receive_handshake_response() {
  std::array<uint8_t, 68> handshake_response;
  co_await asio::async_read(socket_, asio::buffer(handshake_response),
                            asio::as_tuple(asio::use_awaitable));
  // std::cout << "Received handshake response!!!!!" << std::endl;
  auto peer_id_op =
      message_util::parse_handshake_response(handshake_response, info_hash_);
  if (peer_id_op.has_value()) {
    peer_id_ = peer_id_op.value();
  } else {
    throw ParseHandshakeException("could not parse handshake response");
  }
}

asio::awaitable<void> Peer::send_interested() {
  auto interested_message =
      message_util::construct_message(message_util::MessageType::Interested);

  co_await asio::async_write(socket_, asio::buffer(interested_message),
                             asio::as_tuple(asio::use_awaitable));
  std::cout << "Sent interested message" << std::endl;
}

void Peer::set_read_timeout(int num_seconds, asio::steady_timer& timer) {
  timer.expires_from_now(std::chrono::seconds(num_seconds));
  timer.async_wait([this](const asio::error_code& ec) {
    if (!ec) {
      std::cout
          << "!!!!!!!!!!!!!!CANCELLING ALL READ OPS AND DISCONNECTING!!!!!!!!"
          << std::endl;
      continue_connection_ = false;
      socket_.cancel();
    }
  });
}

asio::awaitable<void> Peer::receive_messages() {
  using namespace message_util;
  std::vector<uint8_t> header(4);

  // Keep attempting to read messages until download is complete
  while (continue_connection_ && piece_manager_.continue_download()) {
    // std::cout << "in receive message loop" << std::endl;
    asio::steady_timer timer(socket_.get_executor());

    int timeout_seconds = 10;
    if (is_choked_) {  // give 40s to be unchoked
      timeout_seconds = 40;
    }

    // Read header to get message length
    uint32_t message_length = co_await read_header(timer, header);
    // Handle keep-alive message
    if (message_length == 0) {
      std::cout << "Received keep-alive message" << std::endl;
      continue;
    }

    // Read message type
    uint8_t message_type;

    set_read_timeout(3, timer);
    co_await asio::async_read(socket_, asio::buffer(&message_type, 1),
                              asio::as_tuple(asio::use_awaitable));

    MessageType type = static_cast<MessageType>(message_type);

    uint32_t payload_length = message_length - 1;

    // Read payload
    std::vector<uint8_t> payload(payload_length);

    set_read_timeout(3, timer);
    co_await asio::async_read(socket_, asio::buffer(payload),
                              asio::as_tuple(asio::use_awaitable));

    timer.cancel();

    handle_message(type, payload);
  }

  std::cout << "Completed in receive message" << std::endl;
}

asio::awaitable<uint32_t> Peer::read_header(asio::steady_timer& timer,
                                            std::vector<uint8_t>& header) {
  int timeout_seconds = 10;
  if (is_choked_) {  // give 40s to be unchoked
    timeout_seconds = 40;
  }

  set_read_timeout(timeout_seconds, timer);
  co_await asio::async_read(socket_, asio::buffer(header),
                            asio::as_tuple(asio::use_awaitable));

  co_return message_util::get_header_length(header);
}

void Peer::handle_message(message_util::MessageType type,
                          const std::vector<uint8_t>& payload) {
  using namespace message_util;
  // Handle different message types
  switch (type) {
    case MessageType::Choke:
      std::cout << "Received choke message" << std::endl;
      break;
    case MessageType::Unchoke:
      //  std::cout << "Received unchoke message" << std::endl;
      // Handle unchoke message
      is_choked_ = false;
      break;
    case MessageType::Interested:
      //  std::cout << "Received interested message" << std::endl;
      // Handle interested message
      break;
    case MessageType::NotInterested:
      //  std::cout << "Received not interested message" << std::endl;
      // Handle not interested message
      break;
    case MessageType::Have:
      // std::cout << "Received have message" << std::endl;
      // Handle have message
      break;
    case MessageType::Bitfield:
      std::cout << "Received bitfield message" << std::endl;
      handle_bitfield_message(payload);
      // Handle bitfield message
      break;
    case MessageType::Request:
      //  std::cout << "Received request message" << std::endl;
      // Handle request message
      break;
    case MessageType::Piece:
      //  std::cout << "Received piece message" << std::endl;
      // Handle piece message
      handle_piece_message(payload);
      break;
    case MessageType::Cancel:
      //  std::cout << "Received cancel message" << std::endl;
      // Handle cancel message
      break;
    case MessageType::Port:
      //   std::cout << "Received port message" << std::endl;
      // Handle port message
      break;
    default:
      break;
      // std::cout << "error unknown message type" << std::endl;
  }
}

asio::awaitable<void> Peer::send_messages() {
  while (continue_connection_ && piece_manager_.continue_download()) {
    try {
      co_await send_block_requests();
    } catch (const std::exception& e) {
      std::cout << "error during send block requests to peer " << peer_num_id_
                << ": " << e.what() << std::endl;
      continue_connection_ = false;
      co_return;
    }

    co_await asio::steady_timer(socket_.get_executor(),
                                std::chrono::milliseconds(200))
        .async_wait(asio::use_awaitable);
  }

  std::cout << "Completed in send_messages" << std::endl;
  co_return;
}

void Peer::handle_bitfield_message(const std::vector<uint8_t>& payload) {
  // Calculate the number of pieces based on the payload length
  // size_t payload_length = payload.size();
  // uint32_t num_pieces = payload_length * 8;

  auto peer_bitfield = message_util::get_peer_bitfield(payload, num_pieces_);
  if (peer_bitfield.size() != num_pieces_) {
    std::cout << "received wrong peer bitfield size: " << peer_bitfield.size()
              << " num pieces: " << num_pieces_ << std::endl;
    return;
  }
  // Handle the parsed bitfield data (e.g., store it, update the state, or start
  // requesting pieces)
  piece_manager_.update_piece_frequencies(peer_bitfield, peer_num_id_);
}

asio::awaitable<void> Peer::send_block_requests() {
  // keep attempting to send messages
  if (is_choked_) {
    // std::cout << "i am choked!" << std::endl;
    co_return;
  }
  // if (num_in_flight_ == MAX_IN_FLIGHT) {
  //   std::cout << "skipping peer: " << peer_num_id_ << std::endl;
  // }
  // std::cout << "no issue in send block requests" << std::endl;
  while (num_in_flight_ < MAX_IN_FLIGHT) {
    // std::cout << "finding something to send" << std::endl;
    auto block_request = piece_manager_.select_next_block(peer_num_id_);
    // std::cout << "found something!" << std::endl;
    if (!block_request.has_value()) {
      //  std::cout << "nothing to send" << std::endl;
      co_return;
      // break;
    }
    // std::cout << "sending block request: " <<
    // block_request.value().piece_index
    //           << " offset: " << block_request.value().block_offset
    //           << " peer: " << peer_num_id_ << std::endl;

    auto request_message =
        message_util::construct_request_message(block_request.value());
    co_await asio::async_write(socket_, asio::buffer(request_message),
                               asio::as_tuple(asio::use_awaitable));

    num_in_flight_++;
  }
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
  piece_manager_.add_block(peer_num_id_, block);

  num_in_flight_--;
  return true;
}

uint32_t Peer::get_id() const { return peer_num_id_; }

}  // namespace simpletorrent