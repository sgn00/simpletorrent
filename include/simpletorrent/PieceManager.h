#pragma once

#include <readerwriterqueue.h>

#include <fstream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Buffer.h"
#include "Constant.h"
#include "FastRandom.h"
#include "Metadata.h"

namespace simpletorrent {

class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file, uint32_t num_peers);

  std::optional<BlockRequest> select_next_block(
      uint32_t peer_id);  // given a peer, we need to find the rarest piece
                          // (which the peer has), and find a block which has
                          // not yet been downloaded

  bool add_block(
      uint32_t peer_id,
      const Block& block);  // Peer calls this when a block is downloaded

  bool is_download_complete() const;

  void update_piece_frequencies(
      const std::vector<uint8_t>& peer_bitfield,
      int peer_id);  // on peer connection, feed this bitfield to know
                     // which peer has what piece

  ~PieceManager();

 private:
  static constexpr uint32_t DEFAULT_BLOCK_LENGTH = 16384;  // 16 KiB

  uint32_t piece_length_;

  uint32_t total_length_;

  std::string output_file_;

  std::ofstream output_file_stream_;

  std::mutex pieces_mutex_;

  std::vector<PieceMetadata> pieces_;  // needed for hash checking,

  std::unordered_map<size_t, std::vector<uint8_t>> peers_bitfield_;

  Buffer buffer_;

  std::vector<int> peer_piece_affinity;

  FastRandom rng_;

  uint32_t num_completed_;

  moodycamel::ReaderWriterQueue<std::pair<uint32_t, std::string>> write_queue_;

  std::thread writer_thread_;

  // RarityManager rarity_manager_;

  bool is_verified_piece(uint32_t piece_index, const std::string& data)
      const;  // Check a piece against its hash

  void save_piece(
      uint32_t piece_index,
      const std::string& data);  // Write piece to file asynchronously

  void remove_piece_from_buffer(uint32_t piece_index);

  void remove_affinity(uint32_t piece_index);

  void file_writer();
};

}  // namespace simpletorrent