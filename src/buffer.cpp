#include "simpletorrent/Buffer.h"

namespace simpletorrent {

std::vector<int> Buffer::get_pieces_in_buffer() const {
  std::vector<int> res;
  for (const auto& [k, v] : piece_buffer_map_) {
    res.push_back(k);
  }
  return res;
}

bool Buffer::add_piece_to_buffer(int piece_idx, int num_blocks,
                                 size_t piece_length) {
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
  buffer_[empty_idx] = BufferPiece(piece_idx, num_blocks, piece_length);
  return true;
}

int Buffer::get_block_idx_to_retrieve(int piece_idx) const {
  int buffer_idx = piece_buffer_map_.at(piece_idx);
  const auto& blocks_downloaded = buffer_.at(buffer_idx).blocks_downloaded;
  for (size_t i = 0; i < blocks_downloaded.size(); i++) {
    if (blocks_downloaded.at(i) == DONT_HAVE) {
      return i;
    }
  }
  // maybe should throw exception? should never occur
  return -1;
}

std::optional<std::string> Buffer::write_block_to_buffer(const Block& block) {
  int piece_idx = block.piece_index;
  int block_offset = block.block_offset;
  int block_idx = block_offset / DEFAULT_BLOCK_LENGTH;

  int buffer_idx = piece_buffer_map_.at(piece_idx);
  auto& piece = buffer_[buffer_idx];
  std::copy(block.data.begin(), block.data.end(),
            piece.data.begin() + block_offset);
  piece.blocks_downloaded[block_idx] = HAVE;
  bool completed = std::all_of(piece.blocks_downloaded.cbegin(),
                               piece.blocks_downloaded.cend(),
                               [](uint8_t i) { return i == HAVE; });
  if (!completed) {
    return std::nullopt;
  }
  return std::string(piece.data.begin(), piece.data.end());
}

bool Buffer::has_piece(int piece_idx) const {
  return piece_buffer_map_.count(piece_idx);
}

void Buffer::remove_piece_from_buffer(int piece_idx) {
  int buffer_idx = piece_buffer_map_.at(piece_idx);
  buffer_[buffer_idx].empty = true;
  piece_buffer_map_.erase(buffer_idx);
}

}  // namespace simpletorrent