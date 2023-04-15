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

  BufferPiece(int piece_index, int num_blocks, size_t piece_length)
      : piece_index(piece_index),
        data(piece_length),
        blocks_downloaded(num_blocks, DONT_HAVE),
        empty(false) {}

  int piece_index;
  std::vector<char> data;
  std::vector<uint8_t> blocks_downloaded;
  bool empty;
};

class Buffer {
 public:
  std::vector<int> get_pieces_in_buffer() const;

  bool add_piece_to_buffer(int piece_idx, int num_blocks, size_t piece_length);

  int get_block_idx_to_retrieve(int piece_idx) const;

  std::optional<std::string> write_block_to_buffer(const Block& block);

  bool has_piece(int piece_idx) const;

  void remove_piece_from_buffer(int piece_idx);

 private:
  static constexpr size_t DEFAULT_BUFFER_SIZE =
      64;  // max 64 piece parts in memory
  std::array<BufferPiece, DEFAULT_BUFFER_SIZE> buffer_;

  std::unordered_map<int, int>
      piece_buffer_map_;  // maps piece index to buffer index
};
}  // namespace simpletorrent