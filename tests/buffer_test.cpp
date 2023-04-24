#include "simpletorrent/Buffer.h"

#include "catch2/catch.hpp"

/*
Scenario 1
10 pieces
each piece is 20 bytes
block length is 5 bytes. Each piece has 4 blocks.
total of 5 pieces. Last piece is only 15 bytes.
total file size: 95 bytes

2 peers
*/

using namespace simpletorrent;

TEST_CASE("Buffer init") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(5, 20, Buffer::DEFAULT_BUFFER_SIZE);
  auto ret = buffer.get_pieces_in_buffer();
  REQUIRE(ret.size() == 0);
}

TEST_CASE("Buffer add_piece_to_buffer get_pieces_in_buffer", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);
  uint32_t piece_index = 0;
  REQUIRE(buffer.add_piece_to_buffer(piece_index, piece_length / block_length,
                                     piece_length) == true);

  auto res = buffer.get_pieces_in_buffer();
  REQUIRE(res.size() == 1);
  REQUIRE(res[0] == piece_index);
}

TEST_CASE("Buffer get_pieces_in_buffer", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);

  uint32_t piece_index_1 = 0;
  uint32_t piece_index_2 = 2;

  buffer.add_piece_to_buffer(piece_index_1, piece_length / block_length,
                             piece_length);
  buffer.add_piece_to_buffer(piece_index_2, piece_length / block_length,
                             piece_length);

  auto pieces = buffer.get_pieces_in_buffer();
  REQUIRE(pieces.size() == 2);
  auto bool1 = pieces[0] == piece_index_1 || pieces[1] == piece_index_1;
  REQUIRE(bool1);
  auto bool2 = pieces[1] == piece_index_2 || pieces[0] == piece_index_2;
  REQUIRE(bool2);
}

TEST_CASE(
    "Buffer get_block_index_to_retrieve not requested/completed empty state",
    "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);

  uint32_t piece_index = 0;

  buffer.add_piece_to_buffer(piece_index, piece_length / block_length,
                             piece_length);

  auto [all_requested_or_completed, block_index] =
      buffer.get_block_index_to_retrieve(piece_index);
  REQUIRE(all_requested_or_completed == false);
  REQUIRE(block_index.has_value());
  REQUIRE(block_index.value() == 0);
}

TEST_CASE(
    "Buffer get_block_index_to_retrieve not requested/completed 2 blocks added",
    "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);

  uint32_t piece_index = 0;

  std::vector<uint8_t> dummy_data;

  buffer.add_piece_to_buffer(piece_index, piece_length / block_length,
                             piece_length);

  // Add block0 and block1 to piece index 0
  buffer.write_block_to_buffer(
      Block{piece_index, 0, dummy_data.cbegin(), dummy_data.cend()});
  buffer.write_block_to_buffer(
      Block{piece_index, 5, dummy_data.cbegin(), dummy_data.cend()});

  auto [all_requested_or_completed, block_index] =
      buffer.get_block_index_to_retrieve(piece_index);
  REQUIRE(all_requested_or_completed == false);
  REQUIRE(block_index.has_value());
  REQUIRE(block_index.value() == 2);
}

TEST_CASE("Buffer get_block_index_to_retrieve all requested", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);

  uint32_t piece_index = 0;

  std::vector<uint8_t> dummy_data;

  buffer.add_piece_to_buffer(piece_index, piece_length / block_length,
                             piece_length);

  for (int i = 0; i < 4; i++) {  // requested all 4 blocks
    auto [all_requested_or_completed, block_index] =
        buffer.get_block_index_to_retrieve(piece_index);
    if (i == 3) {
      REQUIRE(all_requested_or_completed == true);
    } else {
      REQUIRE(all_requested_or_completed == false);
    }
    REQUIRE(block_index == i);
  }

  auto [all_requested_or_completed, block_index] =
      buffer.get_block_index_to_retrieve(piece_index);
  REQUIRE(all_requested_or_completed == true);
  REQUIRE(!block_index.has_value());
}

TEST_CASE("Buffer write_block_to_buffer", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);
  std::vector<uint8_t> dummy_data{'a', 'b', 'c', 'd', 'e'};
  std::string expected_data;
  for (int i = 0; i < 4; i++) {
    for (auto j : dummy_data) {
      expected_data += j;
    }
  }

  uint32_t piece_index = 0;

  buffer.add_piece_to_buffer(piece_index, piece_length / block_length,
                             piece_length);

  for (int i = 0; i < 4; i++) {
    auto [completed, data] = buffer.write_block_to_buffer(Block{
        piece_index, i * block_length, dummy_data.cbegin(), dummy_data.cend()});
    if (i != 3) {
      REQUIRE(completed == false);
    } else {
      REQUIRE(completed == true);
      REQUIRE(data.size() == piece_length);
      REQUIRE(data == expected_data);
    }
  }
}

TEST_CASE("Buffer is_full", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);
  for (uint32_t i = 0; i < Buffer::DEFAULT_BUFFER_SIZE; i++) {
    buffer.add_piece_to_buffer(i, piece_length / block_length, piece_length);
  }

  REQUIRE(buffer.is_full());
}

TEST_CASE("Buffer remove_piece_from_buffer", "[Buffer]") {
  uint32_t block_length = 5;
  uint32_t piece_length = 20;
  Buffer buffer(block_length, piece_length, Buffer::DEFAULT_BUFFER_SIZE);
  for (uint32_t i = 0; i < Buffer::DEFAULT_BUFFER_SIZE; i++) {
    buffer.add_piece_to_buffer(i, piece_length / block_length, piece_length);
  }

  buffer.remove_piece_from_buffer(5);
  REQUIRE(!buffer.is_full());

  buffer.remove_piece_from_buffer(3);
  auto pieces_in_buffer = buffer.get_pieces_in_buffer();
  REQUIRE(pieces_in_buffer.size() == Buffer::DEFAULT_BUFFER_SIZE - 2);
}

/*
struct Block {  // this is what the Peer sends us
  const uint32_t piece_index;
  const uint32_t block_offset;
  std::vector<uint8_t>::const_iterator data_begin;
  std::vector<uint8_t>::const_iterator data_end;
};
*/