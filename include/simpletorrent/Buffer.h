#pragma once
#include <array>
#include <unordered_map>
#include <vector>

#include "Constant.h"

namespace simpletorrent {
struct BufferPiece {
  int piece_index;
  std::vector<char> data;
  std::vector<uint8_t> blocks_downloaded;
  bool empty = true;
};

class Buffer {
 public:
  std::vector<int> get_pieces_in_buffer() const;

  bool add_piece_to_buffer(int piece_idx, int num_blocks);

 private:
  static constexpr size_t DEFAULT_BUFFER_SIZE =
      64;  // max 64 piece parts in memory
  std::array<BufferPiece, DEFAULT_BUFFER_SIZE> buffer_;

  std::unordered_map<int, int>
      piece_buffer_map_;  // maps piece index to buffer index
};
}  // namespace simpletorrent