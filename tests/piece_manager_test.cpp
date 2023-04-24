#include "catch2/catch.hpp"
#include "sha1.hpp"
#include "simpletorrent/PieceManager.h"

/*
Scenario 1
10 pieces
each piece is 20 bytes
block length is 5 bytes. Each piece has 4 blocks.
total of 5 pieces. Last piece is only 15 bytes.
total file size: 95 bytes

2 peers
*/

/*
class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);

  std::optional<BlockRequest> select_next_block(uint32_t peer_id);

  void add_block(uint32_t peer_id, const Block& block);

  void set_stop_download();

  bool continue_download() const;

  void update_piece_frequencies(const std::vector<uint8_t>& bitfield,
                                uint32_t peer_id);
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
    res.push_back(normal_hash);
  }

  SHA1 checksum2;
  checksum2.update(last_piece);
  std::string last_hash = checksum.final();
  res.push_back(last_hash);
  return res;
}

TEST_CASE("PieceManager update_piece_frequencies select_next_block",
          "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 10;
  std::string output_file = "piece_manager_test.out";
  PieceManager pm(create_piece_hashes(), piece_length, block_length,
                  total_length, output_file, buffer_size);
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

  pm.set_stop_download();
}

TEST_CASE(
    "PieceManager update_piece_frequencies select_next_block multiple peers",
    "[PieceManager]") {
  size_t piece_length = 20;
  size_t total_length = 95;
  uint32_t block_length = 5;
  uint32_t buffer_size = 10;
  std::string output_file = "piece_manager_test.out";
  PieceManager pm(create_piece_hashes(), piece_length, block_length,
                  total_length, output_file, buffer_size);
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

  pm.set_stop_download();
}

/*
struct BlockRequest {  // this is sent to the Peer

  BlockRequest(uint32_t piece_index, uint32_t block_idx, uint32_t block_length)
      : piece_index(piece_index),
        block_offset(block_idx * block_length),
        block_length(block_length) {}

  const uint32_t piece_index;
  const uint32_t block_offset;
  const uint32_t block_length;
};

*/