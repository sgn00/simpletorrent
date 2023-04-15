#include "simpletorrent/Buffer.h"

#include "catch2/catch.hpp"

/*
Scenario 1
10 pieces
each piece is 20 bytes
block length is 5 bytes
total of 5 pieces. Last piece is only 15 bytes.
total file size: 95 bytes

2 peers
*/

/*

  std::vector<int> get_pieces_in_buffer() const;

  bool add_piece_to_buffer(int piece_idx, int num_blocks, size_t piece_length);

  int get_block_idx_to_retrieve(int piece_idx) const;

  std::optional<std::string> write_block_to_buffer(const Block& block);

  bool has_piece(int piece_idx) const;

  void remove_piece_from_buffer(int piece_idx);
*/

using namespace simpletorrent;

TEST_CASE("Test buffer init") {
  Buffer b;
  auto ret = b.get_pieces_in_buffer();
  REQUIRE(ret.size() == 0);
}