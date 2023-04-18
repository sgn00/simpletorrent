#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Buffer.h"
#include "Constant.h"
#include "Metadata.h"
#include "RarityManager.h"

namespace simpletorrent {

class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);

  std::optional<BlockRequest> select_next_block(
      int peer_id);  // given a peer, we need to find the rarest piece (which
                     // the peer has), and find a block which has not yet been
                     // downloaded

  bool add_block(
      int peer_id,
      const Block& block);  // Peer calls this when a block is downloaded

  bool is_download_complete() const;

  void update_piece_frequencies(
      const std::vector<uint8_t>& peer_bitfield,
      int peer_id);  // on peer connection, feed this bitfield to know
                     // which peer has what piece

 private:
  static constexpr size_t DEFAULT_BLOCK_LENGTH = 16384;  // 16 KiB

  size_t piece_length_;

  size_t total_length_;

  std::string output_file_;

  std::ofstream output_file_stream_;

  std::mutex pieces_mutex_;

  std::vector<PieceMetadata> pieces_;  // needed for hash checking,

  std::unordered_map<int, std::vector<uint8_t>> peers_bitfield_;

  Buffer buffer_;

  // RarityManager rarity_manager_;

  bool is_verified_piece(int piece_index, const std::string& data)
      const;  // Check a piece against its hash

  void save_piece(int piece_index,
                  const std::string& data);  // Write piece to file
};

}  // namespace simpletorrent