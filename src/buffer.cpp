#include "simpletorrent/Buffer.h"

namespace simpletorrent {

std::vector<int> Buffer::get_pieces_in_buffer() const {
  std::vector<int> res;
  for (const auto& [k, v] : piece_buffer_map_) {
    res.push_back(k);
  }
  return res;
}

bool Buffer::add_piece_to_buffer(int piece_idx, int num_blocks) {
  if (piece_buffer_map_.size() == buffer_.size()) {
    return false;
  }

  // find empty index
  int empty_idx = -1;
  for (size_t i = 0; i < buffer_.size(); i++) {
    if (buffer_.at(i).empty) {
      empty_idx = i;
      break;
    }
  }
  buffer_[empty_idx] = BufferPiece{
      piece_idx, {}, std::vector<uint8_t>(num_blocks, DONT_HAVE), false};
  return true;
}

}  // namespace simpletorrent