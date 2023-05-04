#pragma once
#include <readerwriterqueue.h>

#include <fstream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Buffer.h"
#include "FastRandom.h"
#include "FileManager.h"
#include "Metadata.h"

namespace simpletorrent {
class PieceManager {
 public:
  static constexpr uint32_t DEFAULT_BLOCK_LENGTH = 16384;

  // Piece hashes are in binary form
  PieceManager(const TorrentMetadata& data, uint32_t block_length,
               uint32_t buffer_size);

  PieceManager(const PieceManager&) = delete;
  PieceManager& operator=(const PieceManager&) = delete;

  std::optional<BlockRequest> select_next_block(uint32_t peer_id);

  void add_block(uint32_t peer_id, const Block& block);

  bool continue_download() const;

  void update_piece_frequencies(const std::vector<uint8_t>& bitfield,
                                uint32_t peer_id);

  void remove_peer(uint32_t peer_id);

 private:
  uint32_t piece_length_;
  uint32_t block_length_;
  std::vector<PieceMetadata> pieces_;
  Buffer buffer_;
  FastRandom rng_;
  std::unordered_map<uint32_t, uint32_t> peer_piece_affinity_map_;
  std::unordered_map<uint32_t, std::vector<PeerPieceState>> peers_bitfield_map_;
  uint32_t num_pieces_completed_;
  FileManager file_manager_;

  bool is_valid_block_data(const Block& block) const;

  bool is_verified_piece(uint32_t piece_index, const std::string& data) const;

  void save_piece(uint32_t piece_index, const std::string& data);

  void remove_piece_from_buffer(uint32_t piece_index);

  void remove_affinity(uint32_t piece_index);

  std::optional<uint32_t> handle_buffer_not_full_case(uint32_t peer_id);

  std::optional<uint32_t> handle_buffer_full_or_no_piece_to_add(
      uint32_t peer_id);

  uint32_t calculate_block_length(uint32_t piece_index,
                                  uint32_t block_idx_to_retrieve) const;
};

}  // namespace simpletorrent