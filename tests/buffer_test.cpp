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

// TEST_CASE("Add piece to buffer") {
//   Buffer b(5);
//   b.add_piece_to_buffer(0, 4, 20);
//   auto ret = b.get_pieces_in_buffer();
//   REQUIRE(ret.size() == 1);
//   REQUIRE(ret[0] == 0);
// }

// TEST_CASE("Add pieces to buffer") {
//   Buffer b(5);
//   b.add_piece_to_buffer(0, 4, 20);
//   b.add_piece_to_buffer(1, 4, 20);
//   auto ret = b.get_pieces_in_buffer();
//   REQUIRE(ret.size() == 2);
// }

// TEST_CASE("Remove piece from buffer") {
//   Buffer b(5);
//   b.add_piece_to_buffer(0, 4, 20);
//   b.remove_piece_from_buffer(0);
//   auto ret = b.get_pieces_in_buffer();
//   REQUIRE(ret.size() == 0);
// }

// TEST_CASE("Remove piece from buffer 2") {
//   Buffer b(5);
//   b.add_piece_to_buffer(0, 4, 20);
//   b.add_piece_to_buffer(1, 4, 20);
//   b.remove_piece_from_buffer(0);
//   auto ret = b.get_pieces_in_buffer();
//   REQUIRE(ret.size() == 1);
//   REQUIRE(ret[0] == 1);
// }

// TEST_CASE("Write block to piece in buffer") {
//   Buffer b(5);
//   std::string s4 = "ABCDE";
//   std::string s1 = "BCDEF";
//   std::string s2 = "CDEFG";
//   std::string s3 = "DEFGH";
//   std::string concat = s1 + s2 + s3 + s4;

//   auto v4 = std::vector<uint8_t>(s4.begin(), s4.end());
//   auto v1 = std::vector<uint8_t>(s1.begin(), s1.end());
//   auto v3 = std::vector<uint8_t>(s3.begin(), s3.end());
//   auto v2 = std::vector<uint8_t>(s2.begin(), s2.end());

//   Block b4{0, 15, v4.begin(), v4.end()};
//   Block b1{0, 0, v1.begin(), v1.end()};
//   Block b3{0, 10, v3.begin(), v3.end()};
//   Block b2{0, 5, v2.begin(), v2.end()};

//   b.add_piece_to_buffer(0, 4, 20);
//   auto [done4, ret4] = b.write_block_to_buffer(b4);
//   REQUIRE(!done4);
//   auto [done1, ret1] = b.write_block_to_buffer(b1);
//   REQUIRE(!done1);
//   auto [done3, ret3] = b.write_block_to_buffer(b3);
//   REQUIRE(!done3);
//   auto [done2, ret2] = b.write_block_to_buffer(b2);
//   REQUIRE(done2);
//   REQUIRE(ret2 == concat);
// }

// TEST_CASE("Get block index to retrieve") {
//   Buffer b(5);
//   std::string s1 = "ABCDE";

//   b.add_piece_to_buffer(0, 4, 20);

//   auto v1 = std::vector<uint8_t>(s1.begin(), s1.end());

//   Block b1{0, 0, v1.begin(), v1.end()};
//   b.write_block_to_buffer(b1);

//   auto ret = b.get_block_index_to_retrieve(0);
//   REQUIRE(ret == 1);  // should ask for block 2, ie. index 1
// }

// TEST_CASE("Get block index to retrieve last block") {
//   Buffer b(5);
//   std::string s4 = "ABCDE";
//   std::string s1 = "BCDEF";
//   std::string s3 = "DEFGH";

//   b.add_piece_to_buffer(0, 4, 20);

//   auto v4 = std::vector<uint8_t>(s4.begin(), s4.end());
//   auto v1 = std::vector<uint8_t>(s1.begin(), s1.end());
//   auto v3 = std::vector<uint8_t>(s3.begin(), s3.end());

//   Block b4{0, 15, v4.begin(), v4.end()};
//   Block b1{0, 0, v1.begin(), v1.end()};
//   Block b3{0, 10, v3.begin(), v3.end()};

//   b.write_block_to_buffer(b4);
//   b.write_block_to_buffer(b1);
//   b.write_block_to_buffer(b3);

//   auto ret = b.get_block_index_to_retrieve(0);
//   REQUIRE(ret == 1);  // should ask for block 2
// }

// TEST_CASE("has piece") {
//   Buffer b(5);
//   b.add_piece_to_buffer(0, 4, 20);
//   REQUIRE(b.has_piece(0));
//   REQUIRE(!b.has_piece(5));

//   REQUIRE(b.get_pieces_in_buffer().size() == 1);
// }