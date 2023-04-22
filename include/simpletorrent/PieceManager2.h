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
#include "Helper.h"
#include "Helper2.h"
#include "Helper3.h"
#include "Helper4.h"
#include "Metadata.h"

namespace simpletorrent {
class PieceManager2 {
 public:
  PieceManager2(const std::vector<std::string>& piece_hashes,
                size_t piece_length, size_t total_length,
                const std::string& output_file, uint32_t max_num_peers);

  std::optional<BlockRequest> select_next_block(uint32_t peer_id);

  bool add_block(uint32_t peer_id, const Block& block);

  bool is_download_complete() const;

  void update_piece_frequencies(const std::vector<uint8_t>& bitfield,
                                uint32_t peer_id);

  ~PieceManager2();

 private:
  static constexpr uint32_t DEFAULT_BLOCK_LENGTH = 16384;

  uint32_t piece_length_;

  Helper helper5;

  std::ofstream output_file_stream_;

  Helper4 helper4;

  std::vector<PieceMetadata> pieces_;

  Helper3 helper3;

  Buffer buffer_;

  FastRandom rng_;

  Helper helper;

  std::unordered_map<uint32_t, uint32_t> peer_piece_affinity_map_;

  Helper2 helper2;

  std::unordered_map<uint32_t, std::vector<uint8_t>> peers_bitfield_map_;

  Helper helper12;

  moodycamel::ReaderWriterQueue<std::pair<uint32_t, std::string>> write_queue_;

  Helper2 helper11;

  std::thread writer_thread_;

  Helper3 helper10;

  uint32_t num_pieces_completed_;

  bool is_verified_piece(uint32_t piece_index, const std::string& data) const;

  void save_piece(uint32_t piece_index, const std::string& data);

  void remove_piece_from_buffer(uint32_t piece_index);

  void remove_affinity(uint32_t piece_index);

  void file_writer();
};

}  // namespace simpletorrent