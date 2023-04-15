#include "sha1.hpp"
#include "simpletorrent/PieceManager.h"
#include "simpletorrent/RarityManager.h"

namespace simpletorrent {

PieceManager::PieceManager(const std::vector<std::string>& piece_hashes,
                           size_t piece_length, size_t total_length,
                           const std::string& output_file)
    : piece_length_(piece_length),
      total_length_(total_length),
      output_file_(output_file) {
  size_t num_pieces = piece_hashes.size();
  pieces_.reserve(num_pieces);
  size_t last_piece_length = total_length % piece_length;

  for (size_t i = 0; i < num_pieces; ++i) {
    size_t current_piece_length =
        (i == num_pieces - 1 && last_piece_length != 0) ? last_piece_length
                                                        : piece_length;
    size_t num_blocks =
        (current_piece_length + DEFAULT_BLOCK_LENGTH - 1) /
        DEFAULT_BLOCK_LENGTH;  // rounding up for integer division

    pieces_.emplace_back(PieceMetadata{piece_hashes[i], current_piece_length,
                                       num_blocks, false});
  }

  output_file_stream_.open(output_file,
                           std::ios::binary | std::ios::in | std::ios::out);
}

bool PieceManager::add_block(int peer_id, const Block& block) {
  int piece_idx = block.piece_index;
  if (!buffer_.has_piece(piece_idx)) {
    return false;
  }

  auto completed_piece = buffer_.write_block_to_buffer(block);
  if (completed_piece.has_value()) {
    if (is_verified_piece(piece_idx, completed_piece.value())) {
      save_piece(piece_idx, completed_piece.value());
    }
  }

  return true;
}

bool PieceManager::is_verified_piece(int piece_index,
                                     const std::string& data) const {
  const auto& hash = pieces_.at(piece_index).piece_hash;
  SHA1 checksum;
  checksum.update(data);
  std::string piece_hash = checksum.final();
  return piece_hash == hash;
}

void PieceManager::save_piece(int piece_index, const std::string& data) {
  size_t file_offset = piece_index * piece_length_;
  output_file_stream_.seekp(file_offset);
  output_file_stream_.write(data.c_str(), data.size());

  // clean up
  pieces_[piece_index].completed = true;
  // rarity_manager_.remove_piece_from_peers(piece_index);
  buffer_.remove_piece_from_buffer(piece_index);
}

void PieceManager::update_piece_frequencies(
    const std::vector<uint8_t>& peer_bitfield, int peer_id) {
  peers_bitfield_[peer_id] = peer_bitfield;
  // rarity_manager_.update_piece_frequencies(peer_bitfield, peer_id);
}

std::optional<BlockRequest> PieceManager::select_next_block(int peer_id) {
  std::optional<int> chosen_piece;
  const auto& bitfield = peers_bitfield_[peer_id];
  bool in_buffer = false;

  // 1. First we try to fulfill from buffer
  const auto& buffered_pieces = buffer_.get_pieces_in_buffer();
  for (int piece_idx : buffered_pieces) {
    if (bitfield[piece_idx] == HAVE) {
      chosen_piece = std::optional<int>(piece_idx);
      in_buffer = true;
      break;
    }
  }

  if (!chosen_piece.has_value()) {
    // 2. If no match, we try to get rarest piece for this peer
    // chosen_piece = rarity_manager_.select_rarest_piece_for_peer(peer_id);

    // 3. If still no match, we try to get a random piece that has not yet been
    // downloaded
    // if (!chosen_piece.has_value()) {
    for (size_t i = 0; i < bitfield.size(); i++) {
      if (pieces_.at(i).completed == false && bitfield[i] == HAVE) {
        chosen_piece = std::optional<int>(i);
      }
    }
    // }
  }

  // 4. Now check if we actually have a piece, if still no piece, the peer will
  // have nothing to do
  if (!chosen_piece.has_value()) {
    return std::nullopt;
  }

  int piece_idx = chosen_piece.value();
  // Now we have a chosen piece, check if we need to add to buffer
  if (!in_buffer) {
    bool added_to_buffer =
        buffer_.add_piece_to_buffer(piece_idx, pieces_[piece_idx].num_blocks,
                                    pieces_[piece_idx].piece_length);
    if (!added_to_buffer) {  // if buffer is full, then peer will have nothing
                             // to do
      return std::nullopt;
    }
  }

  int block_idx_to_retrieve = buffer_.get_block_idx_to_retrieve(
      piece_idx);  // retrieve the block_idx that has not yet been downloaded
  BlockRequest block_request(piece_idx, block_idx_to_retrieve);
  return block_request;
}
}  // namespace simpletorrent