#include <fstream>

#include "catch2/catch.hpp"
#include "sha1.hpp"
#include "simpletorrent/GlobalState.h"
#include "simpletorrent/PieceManager.h"
#include "simpletorrent/Util.h"

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

std::vector<std::string> create_piece_hashes() {
  std::vector<std::string> res;
  std::string normal_piece = "ABCDEFGHIJK123456789";
  std::string last_piece = "ABCDEFGHIJK1234";

  SHA1 checksum;
  checksum.update(normal_piece);
  std::string normal_hash = checksum.final();

  for (int i = 0; i < 4; i++) {
    res.push_back(hex_decode(normal_hash));
  }

  SHA1 checksum2;
  checksum2.update(last_piece);
  std::string last_hash = checksum.final();
  res.push_back(hex_decode(last_hash));
  return res;
}

TorrentMetadata get_torrent_metadata(
    uint32_t piece_length, long long total_length,
    const std::vector<std::string>& hashes = create_piece_hashes()) {
  return TorrentMetadata{
      {},    hashes, piece_length, total_length, "piece_manager_test.out",
      "XXX", {}};
}

TEST_CASE("PieceManager update_piece_frequencies select_next_block",
          "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 10;
  PieceManager pm(get_torrent_metadata(piece_length, total_length),
                  block_length, buffer_size);
  std::vector<uint8_t> bitfield_0{1, 0, 1, 0, 1};

  pm.update_piece_frequencies(bitfield_0, 0);

  for (int i = 0; i < 4; i++) {
    auto res_op = pm.select_next_block(0);
    REQUIRE(res_op.has_value());
    auto res = res_op.value();
    REQUIRE(res.piece_index == 0);
    REQUIRE(res.block_offset == i * 5);
    REQUIRE(res.block_length == block_length);
  }

  GlobalState::set_stop_download();
}

TEST_CASE(
    "PieceManager update_piece_frequencies select_next_block multiple peers",
    "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 10;
  PieceManager pm(get_torrent_metadata(piece_length, total_length),
                  block_length, buffer_size);
  std::vector<uint8_t> bitfield_0{1, 0, 1, 0, 1};
  std::vector<uint8_t> bitfield_1{1, 0, 0, 0, 0};

  pm.update_piece_frequencies(bitfield_0, 0);
  pm.update_piece_frequencies(bitfield_1, 1);

  int i = 0;
  for (; i < 2; i++) {
    auto res_op = pm.select_next_block(0);
    REQUIRE(res_op.has_value());
    auto res = res_op.value();
    REQUIRE(res.piece_index == 0);
    REQUIRE(res.block_offset == i * 5);
    REQUIRE(res.block_length == block_length);
  }

  for (; i < 4; i++) {
    auto res_op = pm.select_next_block(1);
    REQUIRE(res_op.has_value());
    auto res = res_op.value();
    REQUIRE(res.piece_index == 0);
    REQUIRE(res.block_offset == i * 5);
    REQUIRE(res.block_length == block_length);
  }

  GlobalState::set_stop_download();
}

TEST_CASE(
    "PieceManager update_piece_frequencies select_next_block multiple peers "
    "buffer full no block request for a peer",
    "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 1;
  PieceManager pm(get_torrent_metadata(piece_length, total_length),
                  block_length, buffer_size);
  // in this test case, buffer size is 1
  // peer 0 arrives and will add piece 0 to buffer
  // peer 1 arrives but cannot add to buffer, and cannot be assigned piece 0,
  // so it gets no block requests
  std::vector<uint8_t> bitfield_0{1, 0, 1, 0, 1};
  std::vector<uint8_t> bitfield_1{0, 1, 0, 0, 0};

  pm.update_piece_frequencies(bitfield_0, 0);
  pm.update_piece_frequencies(bitfield_1, 1);

  auto res = pm.select_next_block(0);

  auto res_2 = pm.select_next_block(1);
  REQUIRE(!res_2.has_value());

  GlobalState::set_stop_download();
}

TEST_CASE(
    "PieceManager update_piece_frequencies select_next_block multiple peers "
    "buffer is full and peer is randomly assigned a piece",
    "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 2;
  PieceManager pm(get_torrent_metadata(piece_length, total_length),
                  block_length, buffer_size);
  // in this test case, buffer size is 2
  // peer 0 arrives and will add piece 0 to buffer
  // peer 1 arrives and will add piece 1 to buffer
  // peer 2 arrives, buffer is full so it will either select piece 0, or 1
  // pseudorandomly
  std::vector<uint8_t> bitfield_0{1, 0, 1, 0, 1};
  std::vector<uint8_t> bitfield_1{0, 1, 0, 0, 0};
  std::vector<uint8_t> bitfield_2{1, 1, 0, 0, 0};

  pm.update_piece_frequencies(bitfield_0, 0);
  pm.update_piece_frequencies(bitfield_1, 1);
  pm.update_piece_frequencies(bitfield_2, 2);

  auto res = pm.select_next_block(0);
  REQUIRE(res.value().piece_index == 0);
  auto res_2 = pm.select_next_block(1);
  REQUIRE(res_2.value().piece_index == 1);
  auto res_3 = pm.select_next_block(2);
  REQUIRE(res_3.value().piece_index ==
          1);  // pseudorandom, but seed same so result always same

  GlobalState::set_stop_download();
}

TEST_CASE("PieceManager add_block complete one piece", "[PieceManager]") {
  GlobalState::set_continue_download();
  size_t piece_length = 20;
  size_t total_length = 20;
  uint32_t block_length = 5;
  uint32_t buffer_size = 1;
  std::string output_file = "piece_manager_test.out";
  std::string normal_piece = "ABCDEFGHIJK123456789";
  SHA1 checksum;
  checksum.update(normal_piece);
  std::string normal_hash = checksum.final();
  {
    std::vector<std::string> hash = {hex_decode(normal_hash)};
    PieceManager pm(get_torrent_metadata(piece_length, total_length, hash),
                    block_length, buffer_size);

    std::vector<uint8_t> bitfield_0;
    bitfield_0.push_back(1);
    pm.update_piece_frequencies(bitfield_0, 0);

    for (int i = 0; i < 4; i++) {
      auto res = pm.select_next_block(0);
      REQUIRE(res.value().piece_index == 0);
      REQUIRE(res.value().block_offset == i * block_length);
    }

    for (int i = 0; i < 4; i++) {
      std::vector<uint8_t> block_data;
      for (int k = 0; k < block_length; k++) {
        block_data.push_back(normal_piece[i * block_length + k]);
      }
      std::string temp;
      for (auto i : block_data) {
        temp.push_back(i);
      }
      pm.add_block(0, Block{0, i * block_length, block_data.cbegin(),
                            block_data.cend()});
    }
  }

  std::ifstream inputFile(output_file,
                          std::ios::binary);  // Open the file in binary mode
  std::string result;

  if (inputFile.is_open()) {
    for (int i = 0; i < piece_length && inputFile; ++i) {
      char c;
      inputFile.get(c);
      result.push_back(c);
    }
  }

  REQUIRE(result == normal_piece);
}
