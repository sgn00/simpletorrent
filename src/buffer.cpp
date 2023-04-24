#include "simpletorrent/Buffer.h"

#include <cassert>
#include <iostream>

#include "simpletorrent/FastRandom.h"

namespace simpletorrent {

Buffer::Buffer(uint32_t block_length, uint32_t piece_length,
               uint32_t buffer_size)
    : block_length_(block_length), normal_piece_length_(piece_length) {
  buffer_.reserve(buffer_size);
  uint32_t normal_num_blocks = piece_length / block_length;
  for (size_t i = 0; i < buffer_size; i++) {
    buffer_.push_back(BufferPiece(normal_num_blocks, normal_piece_length_));
  }
}

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

  // find empty index in buffer
  int empty_idx = -1;
  for (size_t i = 0; i < buffer_.size(); i++) {
    if (buffer_.at(i).empty) {
      empty_idx = i;
      break;
    }
  }

  // set up this buffer piece properly
  auto& buffer_piece = buffer_[empty_idx];
  buffer_piece.blocks_state.resize(num_blocks);
  buffer_piece.piece_index = piece_index;
  buffer_piece.data.resize(piece_length);
  buffer_piece.empty = false;
  for (size_t i = 0; i < buffer_piece.blocks_state.size(); i++) {
    buffer_piece.blocks_state[i] = BlockState::DONT_HAVE;
  }

  // map to our piece_buffer_map
  piece_buffer_map_[piece_index] = empty_idx;
  return true;
}

std::pair<bool, std::optional<uint32_t>> Buffer::get_block_index_to_retrieve(
    uint32_t piece_index) {
  assert(piece_buffer_map_.count(piece_index) == 1);
  uint32_t buffer_index = piece_buffer_map_.at(piece_index);
  auto& blocks_downloaded = buffer_.at(buffer_index).blocks_state;

  std::optional<uint32_t> chosen_block = std::nullopt;
  for (size_t i = 0; i < blocks_downloaded.size(); i++) {
    if (blocks_downloaded.at(i) == BlockState::DONT_HAVE) {
      chosen_block = i;
      blocks_downloaded[i] = BlockState::REQUESTED;
      break;
    }
  }

  bool all_requested_or_completed = std::all_of(
      blocks_downloaded.begin(), blocks_downloaded.end(), [](auto& b) {
        return b == BlockState::REQUESTED || b == BlockState::HAVE;
      });

  return {all_requested_or_completed, chosen_block};
}

std::pair<bool, const std::string&> Buffer::write_block_to_buffer(
    const Block& block) {
  auto piece_index = block.piece_index;
  auto block_offset = block.block_offset;
  auto block_index = block_offset / block_length_;

  auto buffer_index = piece_buffer_map_.at(piece_index);
  auto& piece = buffer_[buffer_index];
  // Copy from uint8_t vector to std::string
  std::copy(block.data_begin, block.data_end,
            piece.data.begin() + block_offset);
  piece.blocks_state[block_index] = BlockState::HAVE;
  bool completed =
      std::all_of(piece.blocks_state.cbegin(), piece.blocks_state.cend(),
                  [](BlockState i) { return i == BlockState::HAVE; });

  if (!completed) {
    return {false, piece.data};
  }
  return {true, piece.data};
}

bool Buffer::should_write_block(uint32_t block_offset,
                                uint32_t piece_index) const {
  uint32_t block_index = block_offset / block_length_;
  if (piece_buffer_map_.count(piece_index)) {
    auto buffer_index = piece_buffer_map_.at(piece_index);
    auto block_state = buffer_.at(buffer_index).blocks_state.at(block_index);
    return block_state == BlockState::REQUESTED ||
           block_state == BlockState::DONT_HAVE;
  } else {
    return false;
  }
}

void Buffer::remove_piece_from_buffer(uint32_t piece_index) {
  assert(piece_buffer_map_.count(piece_index) == 1);
  int buffer_index = piece_buffer_map_.at(piece_index);
  buffer_[buffer_index].empty = true;
  piece_buffer_map_.erase(piece_index);
}

bool Buffer::is_full() { return piece_buffer_map_.size() == buffer_.size(); }

void Buffer::clear_all_requested(uint32_t piece_index) {
  assert(piece_buffer_map_.count(piece_index) == 1);
  int buffer_index = piece_buffer_map_.at(piece_index);
  auto& vec = buffer_[buffer_index].blocks_state;
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i] == BlockState::REQUESTED) {
      vec[i] = BlockState::DONT_HAVE;
    }
  }
}

}  // namespace simpletorrent