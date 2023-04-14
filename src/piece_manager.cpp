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

    pieces_.emplace_back(PieceMetadata{piece_hashes[i], false});
  }

  output_file_stream_.open(output_file,
                           std::ios::binary | std::ios::in | std::ios::out);
}

void PieceManager::update_piece_frequencies(
    const std::vector<uint8_t>& peer_bitfield, int peer_id) {
  peers_bitfield_[peer_id] = peer_bitfield;
  rarity_manager_.update_piece_frequencies(peer_bitfield, peer_id);
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
    chosen_piece = rarity_manager_.select_rarest_piece_for_peer(peer_id);

    // 3. if still no match, we try to get a random piece
    if (!chosen_piece.has_value()) {
      if (!chosen_piece.has_value()) {
        for (size_t i = 0; i < bitfield.size(); i++) {
          if (bitfield[i] == HAVE) {
            chosen_piece = std::optional<int>(i);
          }
        }
      }
    }
  }

  // 2. now check if we actually have a piece
  if (!chosen_piece.has_value()) {
    return std::nullopt;
  }

  // now we have a chosen piece, check which block to request from
}

}  // namespace simpletorrent