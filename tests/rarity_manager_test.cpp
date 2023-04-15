// #include "catch2/catch.hpp"
// #include "simpletorrent/RarityManager.h"

// /*
// Scenario 1
// 10 pieces
// each piece is 20 bytes
// block length is 5 bytes
// total of 5 pieces. Last piece is only 15 bytes.
// total file size: 95 bytes

// 2 peers
// */

// using namespace simpletorrent;

// void setup_scenario1(RarityManager& r) {
//   // Peer 1 has piece 0, 4, 2
//   // Peer 2 has piece 0, 4, 3, 1
//   std::vector<uint8_t> p1(5, 0);
//   std::vector<uint8_t> p2(5, 0);
//   p1[0] = 1;
//   p1[4] = 1;
//   p1[2] = 1;

//   p2[0] = 1;
//   p2[4] = 1;
//   p2[3] = 1;
//   p2[1] = 1;

//   r.update_piece_frequencies(p1, 1);
//   r.update_piece_frequencies(p2, 2);
// }

// TEST_CASE("Update then select rarest piece") {
//   // we expect rarest piece for p1 to be 2, p2 to be 1 (ordered)
//   RarityManager r;
//   setup_scenario1(r);
//   auto p1_rarest = r.select_rarest_piece_for_peer(1);
//   REQUIRE(p1_rarest.value() == 2);
//   auto p2_rarest = r.select_rarest_piece_for_peer(2);
//   REQUIRE(p2_rarest.value() == 1);
// }

// TEST_CASE("Remove piece") {
//   // remove piece 1 and 2 to simulate download has finished
//   RarityManager r;
//   setup_scenario1(r);
//   r.remove_piece_from_peers(1);
//   r.remove_piece_from_peers(2);
//   auto p1_rarest = r.select_rarest_piece_for_peer(1);
//   REQUIRE(!p1_rarest.has_value());
//   auto p2_rarest = r.select_rarest_piece_for_peer(2);
//   REQUIRE(p2_rarest.value() == 3);
// }
// /*

//   void update_piece_frequencies(const std::vector<uint8_t>& peer_bitfield,
//                                 int peer_id);
//   void remove_piece_from_peers(int piece_index);

//   std::optional<int> select_rarest_piece_for_peer(int peer_id) const;

//   */