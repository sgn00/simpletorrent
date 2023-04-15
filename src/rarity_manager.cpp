#include <climits>

#include "simpletorrent/RarityManager.h"

namespace simpletorrent {
void RarityManager::remove_piece_from_peers(int piece_index) {
  // assume piece_index is valid
  const auto& peers = piece_to_peers_[piece_index];
  for (int peer_id : peers) {
    rarest_pieces_per_peer_[peer_id].erase(piece_index);
  }

  // Remove the piece from peers_with_piece.
  piece_to_peers_.erase(piece_index);
}

std::optional<int> RarityManager::select_rarest_piece_for_peer(
    int peer_id) const {
  // assume peer id is valid
  const auto& pieces = rarest_pieces_per_peer_.at(peer_id);
  if (pieces.empty()) {
    return std::nullopt;
  } else {
    return *pieces.begin();
  }
}

void RarityManager::update_piece_frequencies(
    const std::vector<uint8_t>& peer_bitfield, int peer_id) {
  for (size_t i = 0; i < peer_bitfield.size(); i++) {
    if (peer_bitfield[i] == DONT_HAVE) {
      continue;
    }
    int piece = i;
    piece_to_peers_[piece].insert(peer_id);
  }

  rarest_pieces_per_peer_[peer_id] = calculate_rarest_pieces_for_peer(peer_id);

  recalculate_rarest_pieces();
}

void RarityManager::recalculate_rarest_pieces() {
  for (auto& [peer_id, rarest_pieces] : rarest_pieces_per_peer_) {
    rarest_pieces = calculate_rarest_pieces_for_peer(peer_id);
  }
}

std::set<int> RarityManager::calculate_rarest_pieces_for_peer(int peer_id) {
  std::set<int> rarest_pieces;
  int min_frequency = INT_MAX;

  for (const auto& piece : piece_to_peers_[peer_id]) {
    int frequency = piece_to_peers_.at(piece).size();
    if (frequency < min_frequency) {
      min_frequency = frequency;
      rarest_pieces.clear();
      rarest_pieces.insert(piece);
    } else if (frequency == min_frequency) {
      rarest_pieces.insert(piece);
    }
  }

  return rarest_pieces;
}

}  // namespace simpletorrent