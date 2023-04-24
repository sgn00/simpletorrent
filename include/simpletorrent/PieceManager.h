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
#include "Metadata.h"

namespace simpletorrent {
class PieceManager {
 public:
  // Piece hashes are in binary form
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, uint32_t block_length, size_t total_length,
               const std::string& output_file, uint32_t buffer_size);

  std::optional<BlockRequest> select_next_block(uint32_t peer_id);

  void add_block(uint32_t peer_id, const Block& block);

  void set_stop_download();

  bool continue_download() const;

  void update_piece_frequencies(const std::vector<uint8_t>& bitfield,
                                uint32_t peer_id);

  static constexpr uint32_t DEFAULT_BLOCK_LENGTH = 16384;

  ~PieceManager();

 private:
  uint32_t piece_length_;

  uint32_t block_length_;

  std::ofstream output_file_stream_;

  std::vector<PieceMetadata> pieces_;

  Buffer buffer_;

  FastRandom rng_;

  std::unordered_map<uint32_t, uint32_t> peer_piece_affinity_map_;

  std::unordered_map<uint32_t, std::vector<PeerPieceState>> peers_bitfield_map_;

  moodycamel::ReaderWriterQueue<std::pair<uint32_t, std::string>> write_queue_;

  std::thread writer_thread_;

  uint32_t num_pieces_completed_;

  bool stop_download;

  bool is_valid_block_data(const Block& block) const;

  bool is_verified_piece(uint32_t piece_index, const std::string& data) const;

  void save_piece(uint32_t piece_index, const std::string& data);

  void remove_piece_from_buffer(uint32_t piece_index);

  void remove_affinity(uint32_t piece_index);

  void file_writer();

  std::optional<uint32_t> handle_buffer_not_full_case(uint32_t peer_id);

  std::optional<uint32_t> handle_buffer_full_or_no_piece_to_add(
      uint32_t peer_id);

  uint32_t calculate_block_length(uint32_t piece_index,
                                  uint32_t block_idx_to_retrieve) const;
};

}  // namespace simpletorrent