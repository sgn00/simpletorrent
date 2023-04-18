#include "simpletorrent/PeerManager.h"

namespace simpletorrent {

uint32_t PeerManager::peer_num_id_ = 1;

PeerManager::PeerManager(PieceManager& piece_manager,
                         const std::vector<PeerConnInfo>& peer_ips,
                         const std::string& info_hash,
                         const std::string& our_id)
    : info_hash_(info_hash), our_id_(our_id) {
  for (const auto& peer_conn_info : peer_ips) {
    peers_.push_back(Peer(piece_manager, io_context_, info_hash_, our_id_,
                          peer_conn_info.ip, peer_conn_info.port,
                          peer_num_id_));
    peer_num_id_++;
  }
}

void PeerManager::start() {
  for (auto& peer : peers_) {
    asio::co_spawn(
        io_context_, [&] { return peer.start(); }, asio::detached);
  }
  io_context_.run();
}
}  // namespace simpletorrent
