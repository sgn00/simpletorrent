#include <filesystem>

#include "sha1.hpp"
#include "simpletorrent/GlobalState.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/PieceManager.h"
#include "simpletorrent/Statistics.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

PieceManager::PieceManager(const TorrentMetadata& data, uint32_t block_length,
                           uint32_t buffer_size)
    : piece_length_(data.piece_length),
      block_length_(block_length),
      buffer_(block_length_, piece_length_, buffer_size),
      num_pieces_completed_(0),
      file_manager_(data) {
  size_t num_pieces = data.piece_hashes.size();
  pieces_.reserve(num_pieces);
  size_t last_piece_length = data.total_length % data.piece_length;
  for (size_t i = 0; i < num_pieces; i++) {
    size_t current_piece_length =
        (i == num_pieces - 1 && last_piece_length != 0) ? last_piece_length
                                                        : piece_length_;
    size_t num_blocks =
        (current_piece_length + block_length_ - 1) / block_length_;
    pieces_.emplace_back(
        data.piece_hashes.at(i), static_cast<uint32_t>(current_piece_length),
        static_cast<uint32_t>(num_blocks), PieceState::NOT_STARTED);
  }
  std::cout << "Last piece len: " << last_piece_length << std::endl;
  std::cout << "Num pieces: " << pieces_.size() << std::endl;
}

std::optional<BlockRequest> PieceManager::select_next_block(uint32_t peer_id) {
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
    chosen_piece = handle_buffer_not_full_case(peer_id);
  }

  // 3. if still no chosen piece - either buffer is full, or we have nothing
  // to add to buffer
  if (!chosen_piece.has_value()) {
    chosen_piece = handle_buffer_full_or_no_piece_to_add(peer_id);
  }

  if (!chosen_piece.has_value()) {
    return std::nullopt;
  }

  uint32_t piece_index = chosen_piece.value();
  auto [all_requested_or_completed, block_idx_to_retrieve] =
      buffer_.get_block_index_to_retrieve(piece_index);

  if (all_requested_or_completed) {
    pieces_.at(piece_index).state = PieceState::ALL_REQUESTED;
    remove_affinity(piece_index);
  }

  if (!block_idx_to_retrieve.has_value()) {
    return std::nullopt;
  }

  uint32_t block_len =
      calculate_block_length(piece_index, block_idx_to_retrieve.value());

  BlockRequest block_request(
      piece_index, block_idx_to_retrieve.value() * block_length_, block_len);

  return block_request;
}

bool PieceManager::is_valid_block_data(const Block& block) const {
  auto piece_index = block.piece_index;
  auto block_offset = block.block_offset;
  if (piece_index >= pieces_.size()) {
    return false;
  }
  if (block_offset >= pieces_.at(piece_index).current_piece_length) {
    return false;
  }

  size_t length_remaining =
      pieces_.at(piece_index).current_piece_length - block_offset;
  auto len = block.data_end - block.data_begin;
  return len <= length_remaining;
}

void PieceManager::add_block(uint32_t peer_id, const Block& block) {
  if (!is_valid_block_data(block)) {
    return;
  }

  uint32_t piece_index = block.piece_index;
  if (!buffer_.should_write_block(block.block_offset, piece_index)) {
    return;
  }

  auto [completed, completed_piece] = buffer_.write_block_to_buffer(block);
  if (completed) {
    if (is_verified_piece(piece_index, completed_piece)) {
      save_piece(piece_index, completed_piece);
      pieces_.at(piece_index).state = PieceState::COMPLETED;
      num_pieces_completed_++;
      LOG_INFO("PieceManager: Completed piece {}", piece_index);
      // std::cout << "Num completed piece: " << num_pieces_completed_
      //           << std::endl;

      if (pieces_.size() - num_pieces_completed_ < 5) {
        LOG_INFO("Pieces not yet completed:");
        for (int i = 0; i < pieces_.size(); i++) {
          if (pieces_[i].state != PieceState::COMPLETED) {
            LOG_INFO("Piece {} not yet completed | piece state: {}", i,
                     static_cast<int>(pieces_[i].state));
          }
          if (i == pieces_.size() - 1 &&
              pieces_[i].state != PieceState::NOT_STARTED) {
            auto blockstate = buffer_.get_block_state(i);
            LOG_INFO("Block state for last block:");
            for (int i = 0; i < blockstate.size(); i++) {
              LOG_INFO("Block {} state: {}", i,
                       static_cast<int>(blockstate[i]));
            }
          }
        }
      }

    } else {
      LOG_ERROR("PieceManager: Piece {} has hash mismatch", piece_index);
      pieces_.at(piece_index).state = PieceState::NOT_STARTED;
    }

    remove_piece_from_buffer(piece_index);
  }

  return;
}

void PieceManager::remove_peer(uint32_t peer_id) {
  LOG_INFO("PieceManager: removing Peer {}", peer_id);
  peer_piece_affinity_map_.erase(peer_id);
  peers_bitfield_map_.erase(peer_id);
}

bool PieceManager::continue_download() const {
  return !GlobalState::is_stop_download() &&
         num_pieces_completed_ != pieces_.size();
}

void PieceManager::update_piece_frequencies(
    const std::vector<uint8_t>& bitfield, uint32_t peer_id) {
  std::vector<PeerPieceState> converted(bitfield.size());
  for (size_t i = 0; i < bitfield.size(); i++) {
    if (bitfield[i] == 1) {
      converted[i] = PeerPieceState::HAVE;
    } else {
      converted[i] = PeerPieceState::DONT_HAVE;
    }
  }
  peers_bitfield_map_[peer_id] = converted;
  LOG_INFO("PieceManager: received bitfield for Peer {}", peer_id);
}

bool PieceManager::is_verified_piece(uint32_t piece_index,
                                     const std::string& data) const {
  const auto& hash = pieces_.at(piece_index).piece_hash;
  SHA1 checksum;
  checksum.update(data);
  std::string piece_hash = checksum.final();
  return hex_decode(piece_hash) == hash;
}

void PieceManager::save_piece(uint32_t piece_index, const std::string& data) {
  Statistics::instance().update_piece_completed();
  size_t file_offset =
      static_cast<size_t>(piece_index) * static_cast<size_t>(piece_length_);
  file_manager_.save_piece(file_offset, data);
}

void PieceManager::remove_piece_from_buffer(uint32_t piece_index) {
  remove_affinity(piece_index);
  buffer_.remove_piece_from_buffer(piece_index);
}

void PieceManager::remove_affinity(uint32_t piece_index) {
  for (auto it = peer_piece_affinity_map_.begin();
       it != peer_piece_affinity_map_.end();) {
    if (it->second == piece_index) {
      it = peer_piece_affinity_map_.erase(it);
    } else {
      ++it;
    }
  }
}

std::optional<uint32_t> PieceManager::handle_buffer_not_full_case(
    uint32_t peer_id) {
  std::optional<uint32_t> chosen_piece;
  const auto& bitfield = peers_bitfield_map_.at(peer_id);

  // find a piece in peer bitfield that peer has, and has not been started
  for (size_t i = 0; i < bitfield.size(); i++) {
    if (pieces_.at(i).state == PieceState::NOT_STARTED &&
        bitfield.at(i) == PeerPieceState::HAVE) {
      chosen_piece = std::optional<uint32_t>(i);
      break;
    }
  }

  // if we found a piece, set piece state to be buffered and set peer affinity
  if (chosen_piece.has_value()) {
    auto piece_index = chosen_piece.value();
    buffer_.add_piece_to_buffer(piece_index, pieces_.at(piece_index).num_blocks,
                                pieces_.at(piece_index).current_piece_length);
    pieces_.at(piece_index).state = PieceState::BUFFERED;
    peer_piece_affinity_map_[peer_id] = piece_index;
  }
  return chosen_piece;
}

std::optional<uint32_t> PieceManager::handle_buffer_full_or_no_piece_to_add(
    uint32_t peer_id) {
  std::optional<uint32_t> chosen_piece;
  const auto& bitfield = peers_bitfield_map_.at(peer_id);
  auto pieces_in_buffer = buffer_.get_pieces_in_buffer();
  if (pieces_in_buffer.size() >
      0) {  // if there is at least one piece in buffer
    // do a random pick
    int start_i = rng_(pieces_in_buffer.size());
    int i = start_i;
    do {
      uint32_t piece_index = pieces_in_buffer.at(i);
      if (bitfield.at(piece_index) == PeerPieceState::HAVE) {
        chosen_piece = std::optional<uint32_t>(piece_index);
        peer_piece_affinity_map_[peer_id] = piece_index;
        if (pieces_.at(piece_index).state == PieceState::ALL_REQUESTED) {
          buffer_.clear_all_requested(piece_index);
          pieces_.at(piece_index).state = PieceState::BUFFERED;
        }
        break;
      }
      i = (i + 1) % pieces_in_buffer.size();
    } while (i != start_i);
  }
  return chosen_piece;
}

uint32_t PieceManager::calculate_block_length(
    uint32_t piece_index, uint32_t block_idx_to_retrieve) const {
  uint32_t block_len = block_length_;
  if (piece_index == pieces_.size() - 1) {
    uint32_t last_block_length =
        pieces_.back().current_piece_length % block_length_;
    uint32_t number_of_blocks =
        pieces_.back().current_piece_length / block_length_;
    if (block_idx_to_retrieve < number_of_blocks) {
      if (block_idx_to_retrieve == 60) {
        LOG_INFO("Retrieve last block len A: {}", block_length_);
      }
      return block_length_;
    } else {
      if (block_idx_to_retrieve == 60) {
        LOG_INFO("Retrieve last block len B: {}", last_block_length);
      }
      return last_block_length;
    }
    // auto count_len = pieces_.back().current_piece_length -
    //                  block_idx_to_retrieve * block_length_;
    // if (count_len >= block_length_) {
    //   block_len = block_length_;
    // } else {
    //   block_len = count_len;
    // }
  }
  return block_len;
}

}  // namespace simpletorrent