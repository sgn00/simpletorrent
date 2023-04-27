#include <iostream>

#include "simpletorrent/PeerManager.h"
#include "simpletorrent/StateTransition.h"

namespace simpletorrent {

PeerManager::PeerManager(PieceManager& piece_manager,
                         std::vector<PeerConnInfo> peer_ips,
                         const std::string& info_hash,
                         const std::string& our_id, uint32_t num_pieces)
    : info_hash_(info_hash),
      our_id_(our_id),
      num_pieces_(num_pieces),
      piece_manager_(piece_manager),
      peers_state_(peer_ips.size(), PeerState::NOT_CONNECTED),
      peer_ips_(std::move(peer_ips)) {
  peers_.reserve(peer_ips.size());
  std::cout << "HERE IN PEER MANAGER " << peer_ips.size() << std::endl;
  for (size_t i = 0; i < peer_ips_.size(); i++) {
    uint32_t peer_num_id = i;
    const auto& peer_conn_info = peer_ips_.at(i);
    peers_.push_back(std::make_unique<Peer>(
        piece_manager, io_context_, info_hash_, our_id_, peer_conn_info.ip,
        peer_conn_info.port, i, num_pieces));
    peers_state_[i] = get_next_peer_state(PeerState::NOT_CONNECTED);
    std::cout << "HERE" << std::endl;
    // start peers up to a max of max num connected peers.
    if (i + 1 == MAX_NUM_CONNECTED_PEERS) {
      break;
    }
  }
}

void PeerManager::start() {
  for (auto& peer : peers_) {
    std::cout << "spawning " << peer->get_id() << " !" << std::endl;
    asio::co_spawn(
        io_context_, [&] { return peer->start(); }, asio::detached);
  }

  asio::co_spawn(
      io_context_, [&] { return cleanup_and_open_connections(); },
      asio::detached);

  io_context_.run();

  std::cout << "returned" << std::endl;
}

asio::awaitable<void> PeerManager::cleanup_and_open_connections() {
  while (piece_manager_.continue_download()) {
    co_await asio::steady_timer(io_context_, std::chrono::seconds(10))
        .async_wait(asio::use_awaitable);

    auto remove_iter =
        std::partition(peers_.begin(), peers_.end(),
                       [](const auto& p) { return !p->exited_; });
    std::cout << "REMOVING " << peers_.end() - remove_iter << " peers..."
              << std::endl;
    for (auto it = remove_iter; it != peers_.end(); it++) {
      auto& p = *it;
      auto peer_id = p->get_id();
      auto peer_state = peers_state_.at(peer_id);
      peers_state_.at(peer_id) = get_next_peer_state(peer_state);
    }

    peers_.erase(remove_iter, peers_.end());
    uint32_t num_connected = peers_.size();
    uint32_t old_size = num_connected;
    for (size_t i = 0; i < peer_ips_.size(); i++) {
      if (peers_state_[i] == PeerState::NOT_CONNECTED ||
          peers_state_[i] == PeerState::DISCONNECTED_1) {
        auto new_peer = std::make_unique<Peer>(
            piece_manager_, io_context_, info_hash_, our_id_,
            peer_ips_.at(i).ip, peer_ips_.at(i).port, i, num_pieces_);
        peers_.push_back(std::move(new_peer));
        asio::co_spawn(
            io_context_, [&] { return peers_.back()->start(); },
            asio::detached);
        peers_state_[i] = get_next_peer_state(peers_state_[i]);
        num_connected++;
        if (num_connected >= MAX_NUM_CONNECTED_PEERS) {
          break;
        }
      }
    }
    std::cout << "ADDED " << num_connected - old_size << " peers..."
              << std::endl;
  }
}

}  // namespace simpletorrent
