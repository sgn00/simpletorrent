#include <filesystem>

#include "sha1.hpp"
#include "simpletorrent/PieceManager2.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

PieceManager2::PieceManager2(const std::vector<std::string>& piece_hashes,
                             size_t piece_length, size_t total_length,
                             const std::string& output_file, uint32_t num_peers)
    : piece_length_(piece_length),
      buffer_(DEFAULT_BLOCK_LENGTH, piece_length_),
      num_pieces_completed_(0),
      write_queue_(5) {
  size_t num_pieces = piece_hashes.size();
  pieces_.reserve(num_pieces);
  size_t last_piece_length = total_length % piece_length;
  for (size_t i = 0; i < num_pieces; i++) {
    size_t current_piece_length =
        (i == num_pieces - 1 && last_piece_length != 0) ? last_piece_length
                                                        : piece_length;
    size_t num_blocks = (current_piece_length + DEFAULT_BLOCK_LENGTH - 1) /
                        DEFAULT_BLOCK_LENGTH;
    pieces_.emplace_back(piece_hashes.at(i),
                         static_cast<uint32_t>(current_piece_length),
                         static_cast<uint32_t>(num_blocks), NOT_STARTED);
  }

  // Create the file and resize it
  std::ofstream new_file(output_file, std::ios::binary | std::ios::out);
  new_file.close();
  std::filesystem::resize_file(output_file, total_length);
  output_file_stream_.open(output_file, std::ios::binary | std::ios::out);

  // Spawn writer thread
  writer_thread_ = std::thread([this]() { this->file_writer(); });
}

PieceManager2::~PieceManager2() {
  std::cout << "destroying piece manager 2" << std::endl;
  writer_thread_.join();
  std::cout << "joined writer thread" << std::endl;
}

std::optional<BlockRequest> PieceManager2::select_next_block(uint32_t peer_id) {
  // Check if peer has registered with us
  if (peers_bitfield_map_.count(peer_id) == 0) {
    return std::nullopt;
  }

  std::optional<uint32_t> chosen_piece;

  // 1. Check if peer has an affinity piece
  if (peer_piece_affinity_map_.count(peer_id) == 1) {
    chosen_piece =
        std::optional<uint32_t>(peer_piece_affinity_map_.at(peer_id));
  }

  // 2.If no affinity piece, and buffer is not full, try to add a piece to
  // buffer
  if (!chosen_piece.has_value() && !buffer_.is_full()) {
    const auto& bitfield = peers_bitfield_map_.at(peer_id);
    for (size_t i = 0; i < bitfield.size(); i++) {
      if (pieces_.at(i).state == NOT_STARTED && bitfield.at(i) == HAVE) {
        chosen_piece = std::optional<uint32_t>(i);
        break;
      }
    }

    // we managed to find a piece we want to add to buffer
    if (chosen_piece.has_value()) {
      auto piece_index = chosen_piece.value();
      // add to buffer, change piece state, and set affinity
      buffer_.add_piece_to_buffer(piece_index,
                                  pieces_.at(piece_index).num_blocks,
                                  pieces_.at(piece_index).current_piece_length);
      pieces_.at(piece_index).state = BUFFERED;
      peer_piece_affinity_map_[peer_id] = piece_index;
    }
  }

  // 3. if still no chosen piece - either buffer is full, or we have nothing to
  // add to buffer
  if (!chosen_piece.has_value()) {
    const auto& bitfield = peers_bitfield_map_.at(peer_id);
    auto pieces_in_buffer = buffer_.get_pieces_in_buffer();
    if (pieces_in_buffer.size() >
        0) {  // if there is at least one piece in buffer
      // do a random pick
      int start_i = rng_(pieces_in_buffer.size());
      int i = start_i;
      do {
        uint32_t piece_index = pieces_in_buffer.at(i);
        if (bitfield.at(piece_index) == HAVE) {
          chosen_piece = std::optional<uint32_t>(piece_index);
          peer_piece_affinity_map_[peer_id] = piece_index;
          if (pieces_.at(piece_index).state == ALL_REQUESTED) {
            buffer_.clear_all_requested(piece_index);
            pieces_.at(piece_index).state = BUFFERED;
          }
          break;
        }
        i = (i + 1) % pieces_in_buffer.size();
      } while (i != start_i);
    }
  }

  if (!chosen_piece.has_value()) {
    return std::nullopt;
  }

  uint32_t piece_index = chosen_piece.value();
  auto [all_requested_or_completed, block_idx_to_retrieve] =
      buffer_.get_block_index_to_retrieve(piece_index);

  if (all_requested_or_completed) {
    pieces_.at(piece_index).state = ALL_REQUESTED;
    remove_affinity(piece_index);
  }

  if (!block_idx_to_retrieve.has_value()) {
    return std::nullopt;
  }

  uint32_t block_len = DEFAULT_BLOCK_LENGTH;
  if (piece_index == pieces_.size() - 1) {
    auto count_len = pieces_.back().current_piece_length -
                     block_idx_to_retrieve.value() * DEFAULT_BLOCK_LENGTH;
    if (count_len >= DEFAULT_BLOCK_LENGTH) {
      block_len = DEFAULT_BLOCK_LENGTH;
    } else {
      block_len = count_len;
    }
  }

  BlockRequest block_request(piece_index, block_idx_to_retrieve.value(),
                             block_len);
  return block_request;
}

bool PieceManager2::add_block(uint32_t peer_id, const Block& block) {
  uint32_t piece_index = block.piece_index;
  if (!buffer_.has_piece(piece_index)) {
    return false;
  }

  auto [completed, completed_piece] = buffer_.write_block_to_buffer(block);
  if (completed) {
    if (is_verified_piece(piece_index, completed_piece)) {
      save_piece(piece_index, completed_piece);
      pieces_.at(piece_index).state = COMPLETED;
      num_pieces_completed_++;
      std::cout << "Num completed piece: " << num_pieces_completed_
                << std::endl;
    } else {
      pieces_.at(piece_index).state = NOT_STARTED;
    }

    remove_piece_from_buffer(piece_index);
  }

  return true;
}

bool PieceManager2::is_download_complete() const {
  return num_pieces_completed_ == pieces_.size();
}

void PieceManager2::update_piece_frequencies(
    const std::vector<uint8_t>& bitfield, uint32_t peer_id) {
  assert(bitfield.size() == pieces_.size());
  peers_bitfield_map_[peer_id] = bitfield;
}

bool PieceManager2::is_verified_piece(uint32_t piece_index,
                                      const std::string& data) const {
  const auto& hash = pieces_.at(piece_index).piece_hash;
  SHA1 checksum;
  checksum.update(data);
  std::string piece_hash = checksum.final();
  return hex_decode(piece_hash) == hash;
}

void PieceManager2::save_piece(uint32_t piece_index, const std::string& data) {
  size_t file_offset = piece_index * piece_length_;
  write_queue_.enqueue(std::pair{file_offset, std::move(data)});
}

void PieceManager2::remove_piece_from_buffer(uint32_t piece_index) {
  remove_affinity(piece_index);
  buffer_.remove_piece_from_buffer(piece_index);
}

void PieceManager2::remove_affinity(uint32_t piece_index) {
  for (auto it = peer_piece_affinity_map_.begin();
       it != peer_piece_affinity_map_.end();) {
    if (it->second == piece_index) {
      it = peer_piece_affinity_map_.erase(it);
    } else {
      ++it;
    }
  }
}

void PieceManager2::file_writer() {
  uint32_t write_count = 0;
  while (!is_download_complete() || write_count != pieces_.size()) {
    std::pair<size_t, std::string> value;
    if (write_queue_.try_dequeue(value)) {
      size_t file_offset = value.first;
      output_file_stream_.seekp(file_offset, std::ios::beg);
      output_file_stream_.write(value.second.c_str(), value.second.size());
      write_count++;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

}  // namespace simpletorrent