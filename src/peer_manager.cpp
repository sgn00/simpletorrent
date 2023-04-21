#include <iostream>

#include "simpletorrent/PeerManager.h"

namespace simpletorrent {

uint32_t PeerManager::peer_num_id_ = 0;

PeerManager::PeerManager(PieceManager& piece_manager,
                         const std::vector<PeerConnInfo>& peer_ips,
                         const std::string& info_hash,
                         const std::string& our_id)
    : info_hash_(info_hash), our_id_(our_id), piece_manager_(piece_manager) {
  peers_.reserve(peer_ips.size());
  for (const auto& peer_conn_info : peer_ips) {
    peers_.emplace_back(piece_manager, io_context_, info_hash_, our_id_,
                        peer_conn_info.ip, peer_conn_info.port, peer_num_id_);
    peer_num_id_++;
    // if (peer_num_id_ == 30) {
    //   break;
    // }
  }
}

PeerManager::~PeerManager() {
  std::cout << "destroying peer manager" << std::endl;
}

void PeerManager::start() {
  for (auto& peer : peers_) {
    std::cout << "spawning!" << std::endl;
    asio::co_spawn(
        io_context_, [&] { return peer.start(); }, asio::detached);
  }

  io_context_.run();

  std::cout << "returned" << std::endl;
}
}  // namespace simpletorrent
