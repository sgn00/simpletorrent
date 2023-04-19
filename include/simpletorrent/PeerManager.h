#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Metadata.h"
#include "Peer.h"
#include "PieceManager.h"

namespace simpletorrent {
class PeerManager {
 public:
  PeerManager(PieceManager& piece_manager,
              const std::vector<PeerConnInfo>& peer_ip,
              const std::string& info_hash, const std::string& our_id);

  void start();

 private:
  static uint32_t peer_num_id_;

  std::string info_hash_;

  std::string our_id_;

  std::vector<Peer> peers_;

  asio::io_context io_context_;

  PieceManager& piece_manager_;
};

}  // namespace simpletorrent