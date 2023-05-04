#include <iostream>

#include "simpletorrent/UdpTracker.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {
UdpTracker::UdpTracker(const std::string& tracker_url,
                       const std::string& info_hash, const std::string& our_id,
                       std::unordered_set<std::string>& peer_set,
                       asio::io_context& io_context)
    : tracker_url_(tracker_url),
      socket_(io_context),
      info_hash_(info_hash),
      our_id_(our_id),
      peer_set_(peer_set),
      io_context_(io_context) {}

void UdpTracker::add_peers() {
  asio::co_spawn(
      io_context_,
      [&] -> asio::awaitable<void> {
        try {
          std::string host = extract_host(tracker_url_);
          std::string port = extract_port(tracker_url_);
          asio::ip::udp::resolver resolver(io_context_);
          auto results = co_await resolver.async_resolve(
              asio::ip::udp::v4(), host, port, asio::use_awaitable);
          tracker_endpoint_ = *results.begin();

          socket_.open(asio::ip::udp::v4());
          auto connection_id = co_await send_connect_request();
          co_await send_announce_request(connection_id);
        } catch (const std::exception& e) {
          LOG_ERROR("Failed getting peers from udp tracker at {} | Error: {}",
                    tracker_url_, e.what());
        }
      },
      asio::detached);
}

std::string UdpTracker::get_url() const { return tracker_url_; }

asio::awaitable<uint64_t> UdpTracker::send_connect_request() {
  constexpr uint64_t initial_connection_id = 0x41727101980;  // special bytes
  constexpr uint32_t connect_action = 0;

  std::array<char, 16> connect_request;
  uint32_t transaction_id = random_uint32();

  auto initial_connection_id_nbo = convert_bo(initial_connection_id);
  auto connect_action_nbo = convert_bo(connect_action);
  auto transaction_id_nbo = convert_bo(transaction_id);

  memcpy(connect_request.data(), &initial_connection_id_nbo, 8);
  memcpy(connect_request.data() + 8, &connect_action_nbo, 4);
  memcpy(connect_request.data() + 12, &transaction_id_nbo, 4);

  socket_.send_to(asio::buffer(connect_request), tracker_endpoint_);

  std::array<char, 16> connect_response;
  asio::ip::udp::endpoint sender_endpoint;

  auto error_occurred = std::make_shared<bool>(false);
  asio::steady_timer timer(io_context_);
  timer.expires_from_now(std::chrono::seconds(5));
  timer.async_wait([&](const asio::error_code& error) {
    if (!error) {
      socket_.cancel();
      *error_occurred = true;
    }
  });

  co_await socket_.async_receive_from(asio::buffer(connect_response),
                                      sender_endpoint, asio::use_awaitable);

  if (*error_occurred) {
    throw std::runtime_error("Timeout for udp tracker");
  }

  uint32_t received_action =
      convert_bo(*(reinterpret_cast<uint32_t*>(connect_response.data())));
  uint32_t received_transaction_id =
      convert_bo(*(reinterpret_cast<uint32_t*>(connect_response.data() + 4)));
  if (received_action != connect_action ||
      received_transaction_id != transaction_id) {
    throw std::runtime_error("Failed to receive a valid connect response");
  }

  uint64_t connection_id =
      convert_bo(*(reinterpret_cast<uint64_t*>(connect_response.data() + 8)));
  co_return connection_id;
}

asio::awaitable<void> UdpTracker::send_announce_request(
    uint64_t connection_id) {
  constexpr uint32_t announce_action = 1;
  uint32_t transaction_id = random_uint32();
  std::array<char, 98> announce_request;

  auto connection_id_nbo = convert_bo(connection_id);
  auto announce_action_nbo = convert_bo(announce_action);
  auto transaction_id_nbo = convert_bo(transaction_id);
  constexpr uint64_t zero_64 = 0;
  constexpr uint32_t zero_32 = 0;
  constexpr uint16_t my_port = 80;
  auto my_port_nbo = convert_bo(my_port);

  constexpr uint32_t max_peers_wanted = 200;
  auto max_peers_wanted_nbo = convert_bo(max_peers_wanted);

  memcpy(announce_request.data(), &connection_id_nbo, 8);
  memcpy(announce_request.data() + 8, &announce_action_nbo, 4);
  memcpy(announce_request.data() + 12, &transaction_id_nbo, 4);
  memcpy(announce_request.data() + 16, hex_decode(info_hash_).data(), 20);
  memcpy(announce_request.data() + 36, our_id_.data(), 20);
  memcpy(announce_request.data() + 56, &zero_64, 8);  // downloaded
  memcpy(announce_request.data() + 64, &zero_64, 8);  // left
  memcpy(announce_request.data() + 72, &zero_64, 8);  // uploaded
  memcpy(announce_request.data() + 80, &zero_32, 4);  // event
  memcpy(announce_request.data() + 84, &zero_32, 4);  // IP
  memcpy(announce_request.data() + 88, &max_peers_wanted_nbo, 4);
  memcpy(announce_request.data() + 92, &my_port_nbo, 2);

  socket_.send_to(asio::buffer(announce_request), tracker_endpoint_);

  std::vector<char> announce_response(2048);
  asio::ip::udp::endpoint sender_endpoint;

  auto error_occurred = std::make_shared<bool>(false);

  asio::steady_timer timer(io_context_);
  timer.expires_from_now(std::chrono::seconds(5));
  timer.async_wait([&](const asio::error_code& error) {
    if (!error) {
      socket_.cancel();
      *error_occurred = true;
    }
  });
  auto bytes_received = co_await socket_.async_receive_from(
      asio::buffer(announce_response), sender_endpoint, asio::use_awaitable);
  if (*error_occurred) {
    throw std::runtime_error("Timeout for udp tracker");
  }

  uint32_t received_action =
      convert_bo(*(reinterpret_cast<uint32_t*>(announce_response.data())));
  uint32_t received_transaction_id =
      convert_bo(*(reinterpret_cast<uint32_t*>(announce_response.data() + 4)));
  if (received_action != announce_action ||
      received_transaction_id != transaction_id) {
    throw std::runtime_error("Failed to receive a valid announce response");
  }

  auto num_peers = (bytes_received - 20) / 6;

  int prv_num = peer_set_.size();
  for (size_t i = 0; i < num_peers; i++) {
    asio::ip::address_v4::bytes_type ip_bytes;
    memcpy(ip_bytes.data(), announce_response.data() + 20 + (i * 6), 4);
    uint16_t port = convert_bo(*(reinterpret_cast<std::uint16_t*>(
        announce_response.data() + 20 + (i * 6) + 4)));
    asio::ip::address_v4 ip_address(ip_bytes);
    std::string peer_str = ip_address.to_string() + ":" + std::to_string(port);
    peer_set_.insert(peer_str);
  }

  LOG_INFO("Udp tracker at {} contributed {} peers", tracker_url_,
           peer_set_.size() - prv_num);

  co_return;
}
}  // namespace simpletorrent