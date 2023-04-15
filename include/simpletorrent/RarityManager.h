#pragma once

#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Constant.h"

namespace simpletorrent {

class RarityManager {
 public:
  void update_piece_frequencies(const std::vector<uint8_t>& peer_bitfield,
                                int peer_id);
  void remove_piece_from_peers(int piece_index);

  std::optional<int> select_rarest_piece_for_peer(int peer_id) const;

 private:
  std::unordered_map<int, std::set<int>> rarest_pieces_per_peer_;
  std::unordered_map<int, std::unordered_set<int>> piece_to_peers_;

  void recalculate_rarest_pieces();
  std::set<int> calculate_rarest_pieces_for_peer(int peer_id);
};
}  // namespace simpletorrent