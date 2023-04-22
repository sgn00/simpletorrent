#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Metadata.h"
#include "Peer.h"
#include "PieceManager2.h"

namespace simpletorrent {
class PeerManager {
 public:
  PeerManager(PieceManager2& piece_manager,
              const std::vector<PeerConnInfo>& peer_ip,
              const std::string& info_hash, const std::string& our_id,
              uint32_t num_pieces);

  void start();

  ~PeerManager();

 private:
  static uint32_t peer_num_id_;

  std::string info_hash_;

  std::string our_id_;

  asio::io_context io_context_;

  PieceManager2& piece_manager_;

  std::vector<Peer> peers_;
};

}  // namespace simpletorrent