#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace simpletorrent {

struct BlockRequest {  // this is sent to the Peer

  BlockRequest(uint32_t piece_index, uint32_t block_idx, uint32_t block_length)
      : piece_index(piece_index),
        block_offset(block_idx * block_length),
        block_length(block_length) {}

  const uint32_t piece_index;
  const uint32_t block_offset;
  const uint32_t block_length;
};

struct Block {  // this is what the Peer sends us
  const uint32_t piece_index;
  const uint32_t block_offset;
  const std::vector<uint8_t>& data;
};

struct PieceMetadata {  // this is in our vector of PieceMetadata, for checking
                        // of hash and whether it is completed
  const std::string piece_hash;
  const uint32_t current_piece_length;
  const uint32_t num_blocks;
  bool completed;
};
}  // namespace simpletorrent