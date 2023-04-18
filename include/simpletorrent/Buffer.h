#pragma once
#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Constant.h"
#include "Metadata.h"

namespace simpletorrent {
struct BufferPiece {
  BufferPiece() : empty(true) {}

  BufferPiece(uint32_t piece_index, uint32_t num_blocks, uint32_t piece_length)
      : piece_index(piece_index),
        data(piece_length),
        blocks_downloaded(num_blocks, DONT_HAVE),
        empty(false) {}

  uint32_t piece_index;
  std::vector<uint8_t> data;
  std::vector<uint8_t> blocks_downloaded;
  bool empty;
};

class Buffer {
 public:
  Buffer(uint32_t block_length);

  std::vector<uint32_t> get_pieces_in_buffer() const;

  bool add_piece_to_buffer(uint32_t piece_idx, uint32_t num_blocks,
                           uint32_t piece_length);

  uint32_t get_block_index_to_retrieve(uint32_t piece_index) const;

  std::optional<std::string> write_block_to_buffer(const Block& block);

  bool has_piece(uint32_t piece_idx) const;

  void remove_piece_from_buffer(uint32_t piece_idx);

 private:
  static constexpr uint32_t DEFAULT_BUFFER_SIZE =
      32;  // max 32 piece parts in memory
  std::array<BufferPiece, DEFAULT_BUFFER_SIZE> buffer_;

  std::unordered_map<uint32_t, uint32_t>
      piece_buffer_map_;  // maps piece index to buffer index

  uint32_t block_length_;
};
}  // namespace simpletorrent