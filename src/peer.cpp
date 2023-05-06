#include "simpletorrent/Peer.h"

#include "simpletorrent/Duration.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/MessageUtil.h"
#include "simpletorrent/Statistics.h"
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
      num_pieces_(num_pieces),
      continue_connection_(true),
      socket_(io_context),
      main_routine_has_exited_(false),
      spawned_routine_has_exited_(true) {}

bool Peer::has_exited() const {
  return main_routine_has_exited_ && spawned_routine_has_exited_;
}

asio::awaitable<void> Peer::start() {
  static int peer_connected_count = 0;

  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip_address_), port_);

  try {
    asio::steady_timer timer(socket_.get_executor());
    set_timeout(duration::CONNECT_TIMEOUT, timer);
    co_await socket_.async_connect(endpoint,
                                   asio::as_tuple(asio::use_awaitable));
    timer.cancel();
  } catch (const std::exception& e) {
    LOG_ERROR("Error connecting to peer {} | Error: {}", peer_num_id_,
              e.what());
    main_routine_has_exited_ = true;
    co_return;
  }

  try {
    co_await send_handshake();
  } catch (const std::exception& e) {
    LOG_ERROR("Error during send handshake to peer {} | Error: {}",
              peer_num_id_, e.what());
    main_routine_has_exited_ = true;
    co_return;
  }

  try {
    co_await receive_handshake_response();
  } catch (const std::exception& e) {
    LOG_ERROR("Error during receive handshake from peer {} | Error: {}",
              peer_num_id_, e.what());
    main_routine_has_exited_ = true;
    co_return;
  }

  if (!piece_manager_.continue_download()) {
    main_routine_has_exited_ = true;
    co_return;
  }

  try {
    co_await send_interested();
  } catch (const std::exception& e) {
    LOG_ERROR("Error during send interested to peer {} | Error: {}",
              peer_num_id_, e.what());
    main_routine_has_exited_ = true;
    co_return;
  }

  peer_connected_count++;
  Statistics::instance().update_num_peers(peer_connected_count);

  spawned_routine_has_exited_ = false;
  asio::co_spawn(
      io_context_, [this]() { return send_messages(); }, asio::detached);

  try {
    co_await receive_messages();
  } catch (const std::exception& e) {
    LOG_ERROR("Error during receive messages from peer {} | Error: {}",
              peer_num_id_, e.what());
  }

  peer_connected_count--;
  Statistics::instance().update_num_peers(peer_connected_count);
  continue_connection_ = false;
  main_routine_has_exited_ = true;
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
  asio::steady_timer timer(socket_.get_executor());
  set_timeout(duration::HANDSHAKE_TIMEOUT, timer);
  co_await asio::async_read(socket_, asio::buffer(handshake_response),
                            asio::as_tuple(asio::use_awaitable));
  timer.cancel();
  auto peer_id_op =
      message_util::parse_handshake_response(handshake_response, info_hash_);
  if (peer_id_op.has_value()) {
    peer_id_ = peer_id_op.value();
  } else {
    throw std::runtime_error("could not parse handshake response");
  }
}

asio::awaitable<void> Peer::send_interested() {
  auto interested_message =
      message_util::construct_message(message_util::MessageType::Interested);

  co_await asio::async_write(socket_, asio::buffer(interested_message),
                             asio::as_tuple(asio::use_awaitable));
}

void Peer::set_timeout(uint32_t num_seconds, asio::steady_timer& timer) {
  timer.expires_from_now(std::chrono::seconds(num_seconds));
  timer.async_wait([this](const asio::error_code& ec) {
    if (!ec) {
      LOG_ERROR("Timeout occurred in peer {}", peer_num_id_);
      continue_connection_ = false;
      socket_.cancel();
    }
  });
}

asio::awaitable<void> Peer::receive_messages() {
  using namespace message_util;
  std::vector<uint8_t> header(4);

  while (continue_connection_) {
    asio::steady_timer timer(socket_.get_executor());

    uint32_t message_length = co_await read_header(timer, header);
    // Handle keep-alive message
    if (message_length == 0) {
      continue;
    }

    uint8_t message_type;
    set_timeout(duration::RECV_MSG_TIMEOUT, timer);
    co_await asio::async_read(socket_, asio::buffer(&message_type, 1),
                              asio::as_tuple(asio::use_awaitable));
    timer.cancel();

    MessageType type = static_cast<MessageType>(message_type);

    uint32_t payload_length = message_length - 1;

    std::vector<uint8_t> payload(payload_length);

    set_timeout(duration::RECV_MSG_TIMEOUT, timer);
    co_await asio::async_read(socket_, asio::buffer(payload),
                              asio::as_tuple(asio::use_awaitable));

    timer.cancel();

    handle_message(type, payload);
  }

  LOG_INFO("Exiting receive message in peer {}", peer_num_id_);
}

asio::awaitable<uint32_t> Peer::read_header(asio::steady_timer& timer,
                                            std::vector<uint8_t>& header) {
  uint32_t timeout_seconds = duration::WAIT_MSG_TIMEOUT;
  if (is_choked_) {
    timeout_seconds = duration::CHOKE_TIMEOUT;
  }

  set_timeout(timeout_seconds, timer);
  co_await asio::async_read(socket_, asio::buffer(header),
                            asio::as_tuple(asio::use_awaitable));
  timer.cancel();

  co_return message_util::get_header_length(header);
}

void Peer::handle_message(message_util::MessageType type,
                          const std::vector<uint8_t>& payload) {
  using namespace message_util;
  switch (type) {
    case MessageType::Choke:
      is_choked_ = true;
      break;
    case MessageType::Unchoke:
      is_choked_ = false;
      break;
    case MessageType::Interested:
      break;
    case MessageType::NotInterested:
      break;
    case MessageType::Have:
      break;
    case MessageType::Bitfield:
      handle_bitfield_message(payload);
      break;
    case MessageType::Request:
      break;
    case MessageType::Piece:
      handle_piece_message(payload);
      break;
    case MessageType::Cancel:
      break;
    case MessageType::Port:
      break;
    default:
      break;
  }
}

asio::awaitable<void> Peer::send_messages() {
  while (continue_connection_) {
    try {
      co_await send_block_requests();
    } catch (const std::exception& e) {
      LOG_INFO("Error during send block requests to peer {} | Error: {}",
               peer_num_id_, e.what());
      continue_connection_ = false;
      co_return;
    }

    co_await asio::steady_timer(socket_.get_executor(),
                                std::chrono::milliseconds(100))
        .async_wait(asio::use_awaitable);
  }

  LOG_INFO("Exiting send messages in peer {}", peer_num_id_);
  spawned_routine_has_exited_ = true;
  co_return;
}

void Peer::handle_bitfield_message(const std::vector<uint8_t>& payload) {
  auto peer_bitfield = message_util::get_peer_bitfield(payload, num_pieces_);
  bool no_pieces = std::all_of(peer_bitfield.begin(), peer_bitfield.end(),
                               [](uint8_t value) { return value == 0; });
  if (no_pieces) {
    throw std::runtime_error("peer has no pieces, terminating connection");
  }
  piece_manager_.update_piece_frequencies(peer_bitfield, peer_num_id_);
}

asio::awaitable<void> Peer::send_block_requests() {
  if (is_choked_) {
    co_return;
  }

  while (num_in_flight_ < MAX_IN_FLIGHT) {
    auto block_request = piece_manager_.select_next_block(peer_num_id_);
    if (!block_request.has_value()) {
      LOG_INFO("No block request for peer {}", peer_num_id_);
      co_return;
    }

    auto request_message =
        message_util::construct_request_message(block_request.value());
    co_await asio::async_write(socket_, asio::buffer(request_message),
                               asio::as_tuple(asio::use_awaitable));

    num_in_flight_++;
  }
}

void Peer::handle_piece_message(const std::vector<uint8_t>& payload) {
  if (payload.size() < 9) {
    LOG_ERROR("Malformed piece message (too short) in peer {}", peer_num_id_);
    return;
  }

  auto [piece_index, block_offset] =
      message_util::retrieve_piece_index_and_block_offset(payload);

  Block block{piece_index, block_offset, payload.begin() + 8, payload.end()};
  piece_manager_.add_block(peer_num_id_, block);

  num_in_flight_--;
  return;
}

uint32_t Peer::get_id() const { return peer_num_id_; }

}  // namespace simpletorrent