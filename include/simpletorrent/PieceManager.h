#pragma once

#include <fstream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Buffer.h"
#include "Constant.h"
#include "RarityManager.h"

namespace simpletorrent {

struct BlockRequest {  // this is sent to the Peer
  int piece_index;
  int block_offset;
  int block_length;
};

struct Block {  // this is what the Peer sends us
  int piece_index;
  int block_offset;
  std::vector<char> data;
};

struct PieceMetadata {  // this is in our vector of PieceMetadata, for checking
                        // of hash and whether it is completed
  std::string piece_hash;
  bool completed;
};

class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);

  std::optional<BlockRequest> request_block(int peer_id);

  bool add_block(
      int peer_id,
      const Block& block);  // Peer calls this when a block is downloaded

  bool verify_piece(int piece_index);  // Check a piece against its hash

  void save_piece(int piece_index);  // Write piece to file

  bool is_download_complete() const;

  std::vector<uint8_t> get_bitfield() const;

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

  RarityManager rarity_manager_;

  void update_piece_frequencies(
      const std::vector<uint8_t>& peer_bitfield,
      int peer_id);  // on peer connection, feed this bitfield to know
                     // which peer has what piece

  std::optional<BlockRequest> select_next_block(
      int peer_id);  // given a peer, we need to find the rarest piece (which
                     // the peer has), and find a block which has not yet been
                     // downloaded

  // how do we maintain the rarity, in a way that we know which peer has which
  // piece as well?
};

}  // namespace simpletorrent