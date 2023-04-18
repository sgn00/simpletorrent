#include "simpletorrent/Buffer.h"

namespace simpletorrent {

Buffer::Buffer(uint32_t block_length) : block_length_(block_length) {}

std::vector<uint32_t> Buffer::get_pieces_in_buffer() const {
  std::vector<uint32_t> res;
  for (const auto& [k, v] : piece_buffer_map_) {
    res.push_back(k);
  }
  return res;
}

bool Buffer::add_piece_to_buffer(uint32_t piece_index, uint32_t num_blocks,
                                 uint32_t piece_length) {
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
  buffer_[empty_idx] = BufferPiece(piece_index, num_blocks, piece_length);
  piece_buffer_map_[piece_index] = empty_idx;
  return true;
}

uint32_t Buffer::get_block_index_to_retrieve(uint32_t piece_index) const {
  uint32_t buffer_index = piece_buffer_map_.at(piece_index);
  const auto& blocks_downloaded = buffer_.at(buffer_index).blocks_downloaded;
  for (size_t i = 0; i < blocks_downloaded.size(); i++) {
    if (blocks_downloaded.at(i) == DONT_HAVE) {
      return i;
    }
  }
  // maybe should throw exception? should never occur
  return -1;
}

std::optional<std::string> Buffer::write_block_to_buffer(const Block& block) {
  auto piece_index = block.piece_index;
  auto block_offset = block.block_offset;
  auto block_index = block_offset / block_length_;

  auto buffer_index = piece_buffer_map_.at(piece_index);
  auto& piece = buffer_[buffer_index];
  std::copy(block.data.begin(), block.data.end(),
            piece.data.begin() + block_offset);
  piece.blocks_downloaded[block_index] = HAVE;
  bool completed = std::all_of(piece.blocks_downloaded.cbegin(),
                               piece.blocks_downloaded.cend(),
                               [](uint8_t i) { return i == HAVE; });
  if (!completed) {
    return std::nullopt;
  }
  return std::string(piece.data.begin(), piece.data.end());
}

bool Buffer::has_piece(uint32_t piece_index) const {
  return piece_buffer_map_.count(piece_index);
}

void Buffer::remove_piece_from_buffer(uint32_t piece_index) {
  int buffer_index = piece_buffer_map_.at(piece_index);
  buffer_[buffer_index].empty = true;
  piece_buffer_map_.erase(buffer_index);
}

}  // namespace simpletorrent