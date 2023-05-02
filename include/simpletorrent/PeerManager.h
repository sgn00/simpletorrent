#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Constant.h"
#include "Metadata.h"
#include "Peer.h"
#include "PieceManager.h"

namespace simpletorrent {
class PeerManager {
 public:
  // info hash is in hex form
  PeerManager(PieceManager& piece_manager, std::vector<PeerConnInfo> peer_ip,
              const std::string& info_hash, const std::string& our_id,
              uint32_t num_pieces);

  PeerManager(const PeerManager&) = delete;
  PeerManager& operator=(const PeerManager&) = delete;

  void start();

 private:
  static constexpr uint32_t MAX_NUM_CONNECTED_PEERS = 60;

  std::string info_hash_;

  std::string our_id_;

  uint32_t num_pieces_;

  asio::io_context io_context_;

  PieceManager& piece_manager_;

  std::vector<std::unique_ptr<Peer>> peers_;

  std::vector<PeerState> peers_state_;

  std::vector<PeerConnInfo> peer_ips_;

  asio::awaitable<void> cleanup_and_open_connections();

  void cleanup_connections();

  void open_connections();
};

}  // namespace simpletorrent