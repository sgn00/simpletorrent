#pragma once
#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Constant.h"
#include "FastRandom.h"
#include "Metadata.h"

namespace simpletorrent {
struct BufferPiece {
  uint32_t piece_index;
  std::string data;
  std::vector<BlockState> blocks_state;
  bool empty;

  BufferPiece(uint32_t num_blocks, uint32_t piece_length)
      : piece_index(piece_index),
        data(piece_length, '\0'),
        blocks_state(num_blocks, BlockState::DONT_HAVE),
        empty(true) {}
};

class Buffer {
 public:
  static constexpr uint32_t DEFAULT_BUFFER_SIZE =
      32;  // max 32 piece parts in memory

  Buffer(uint32_t block_length, uint32_t piece_length, uint32_t buffer_size);

  std::vector<uint32_t> get_pieces_in_buffer() const;

  bool add_piece_to_buffer(uint32_t piece_idx, uint32_t num_blocks,
                           uint32_t piece_length);

  std::pair<bool, std::optional<uint32_t>> get_block_index_to_retrieve(
      uint32_t piece_index);

  std::pair<bool, const std::string&> write_block_to_buffer(const Block& block);

  bool should_write_block(uint32_t block_offset, uint32_t piece_index) const;

  void remove_piece_from_buffer(uint32_t piece_index);

  void clear_all_requested(uint32_t piece_index);

  bool is_full();

  // for debug
  std::vector<BlockState> get_block_state(uint32_t piece_index);

 private:
  std::vector<BufferPiece> buffer_;
  std::unordered_map<uint32_t, uint32_t>
      piece_buffer_map_;  // maps piece index to buffer index
  const uint32_t block_length_;
  const uint32_t normal_piece_length_;  // piece length of normal piece (except
                                        // possibly last piece)
  mutable FastRandom rng_;
};
}  // namespace simpletorrent