#pragma once
#include <string>
#include <vector>

namespace simpletorrent {
static constexpr size_t DEFAULT_BLOCK_LENGTH = 16384;  // 16 KiB

struct BlockRequest {  // this is sent to the Peer

  BlockRequest(int piece_index, int block_idx)
      : piece_index(piece_index),
        block_offset(block_idx * DEFAULT_BLOCK_LENGTH),
        block_length(DEFAULT_BLOCK_LENGTH) {}

  int piece_index;
  int block_offset;
  int block_length;
};

struct Block {  // this is what the Peer sends us
  int piece_index;
  int block_offset;
  std::vector<char> data;
};

struct PieceMetadata {  // this is in our vector of PieceMetadata, for checking
                        // of hash and whether it is completed
  std::string piece_hash;
  size_t piece_length;
  size_t num_blocks;
  bool completed;
};
}  // namespace simpletorrent