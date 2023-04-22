#include "simpletorrent/Buffer.h"

#include <iostream>

#include "simpletorrent/FastRandom.h"

namespace simpletorrent {

Buffer::~Buffer() { std::cout << "destroying buffer" << std::endl; }

Buffer::Buffer(uint32_t block_length, uint32_t piece_length)
    : block_length_(block_length), normal_piece_length_(piece_length) {
  buffer_.reserve(DEFAULT_BUFFER_SIZE);
  uint32_t normal_num_blocks = piece_length / block_length;
  for (size_t i = 0; i < DEFAULT_BUFFER_SIZE; i++) {
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

  // find empty index
  int empty_idx = -1;
  for (size_t i = 0; i < buffer_.size(); i++) {
    if (buffer_.at(i).empty) {
      empty_idx = i;
      break;
    }
  }

  // set up this buffer piece properly
  auto& buffer_piece = buffer_[empty_idx];
  buffer_piece.blocks_downloaded.resize(num_blocks);
  buffer_piece.piece_index = piece_index;
  buffer_piece.data.resize(piece_length);
  buffer_piece.empty = false;
  for (size_t i = 0; i < buffer_piece.blocks_downloaded.size(); i++) {
    buffer_piece.blocks_downloaded[i] = DONT_HAVE;
  }

  // map to our piece_buffer_map
  piece_buffer_map_[piece_index] = empty_idx;
  return true;
}

std::pair<bool, std::optional<uint32_t>> Buffer::get_block_index_to_retrieve(
    uint32_t piece_index) {
  uint32_t buffer_index = piece_buffer_map_.at(piece_index);
  auto& blocks_downloaded = buffer_.at(buffer_index).blocks_downloaded;

  std::optional<uint32_t> chosen_block = std::nullopt;
  for (size_t i = 0; i < blocks_downloaded.size(); i++) {
    if (blocks_downloaded.at(i) == DONT_HAVE) {
      chosen_block = i;
      blocks_downloaded[i] = REQUESTED;
      break;
    }
  }

  bool all_requested_or_completed =
      std::all_of(blocks_downloaded.begin(), blocks_downloaded.end(),
                  [](auto& b) { return b == REQUESTED || b == HAVE; });

  return {all_requested_or_completed, chosen_block};

  // size_t start_index = rng_(blocks_downloaded.size());
  // size_t index = start_index;

  // do {
  //   if (blocks_downloaded.at(index) == DONT_HAVE) {
  //     return index;
  //   }

  //   index = (index + 1) % blocks_downloaded.size();
  // } while (index != start_index);

  // // maybe should throw exception? should never occur
  // return std::nullopt;
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
  piece.blocks_downloaded[block_index] = HAVE;
  bool completed = std::all_of(piece.blocks_downloaded.cbegin(),
                               piece.blocks_downloaded.cend(),
                               [](uint8_t i) { return i == HAVE; });
  // std::cout << "Missing blocks for piece " << piece_index << " : ";
  // for (size_t j = 0; j < piece.blocks_downloaded.size(); j++) {
  //   if (piece.blocks_downloaded[j] != HAVE) {
  //     std::cout << j << ",";
  //   }
  // }
  // std::cout << "Completed: " << completed << std::endl;
  // std::cout << std::endl;
  if (!completed) {
    std::string empty;
    return {false, empty};
  }
  return {true, piece.data};
}

bool Buffer::has_piece(uint32_t piece_index) const {
  return piece_buffer_map_.count(piece_index);
}

void Buffer::remove_piece_from_buffer(uint32_t piece_index) {
  int buffer_index = piece_buffer_map_.at(piece_index);
  buffer_[buffer_index].empty = true;
  piece_buffer_map_.erase(piece_index);
}

bool Buffer::is_full() {
  // std::cout << "piece_buffer_map size: " << piece_buffer_map_.size()
  //  << std::endl;
  // std::cout << "buffer size: " << buffer_.size() << std::endl;
  return piece_buffer_map_.size() == buffer_.size();
}

void Buffer::clear_all_requested(uint32_t piece_index) {
  int buffer_index = piece_buffer_map_.at(piece_index);
  auto& vec = buffer_[buffer_index].blocks_downloaded;
  for (size_t i = 0; i < vec.size(); i++) {
    if (vec[i] == REQUESTED) {
      vec[i] = DONT_HAVE;
    }
  }
}

}  // namespace simpletorrent