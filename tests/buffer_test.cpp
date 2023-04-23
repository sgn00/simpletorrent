// #include "simpletorrent/Buffer.h"

// #include "catch2/catch.hpp"

// /*
// Scenario 1
// 10 pieces
// each piece is 20 bytes
// block length is 5 bytes. Each piece has 4 blocks.
// total of 5 pieces. Last piece is only 15 bytes.
// total file size: 95 bytes

// 2 peers
// */

// using namespace simpletorrent;

// TEST_CASE("Test buffer init") {
//   Buffer b(5);
//   auto ret = b.get_pieces_in_buffer();
//   REQUIRE(ret.size() == 0);
// }
