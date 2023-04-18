#pragma once
#include <string>
#include <vector>

namespace simpletorrent {

struct BlockRequest {  // this is sent to the Peer

  BlockRequest(int piece_index, int block_idx, int block_length)
      : piece_index(piece_index),
        block_offset(block_idx * block_length),
        block_length(block_length) {}

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
  size_t current_piece_length;
  size_t num_blocks;
  bool completed;
};
}  // namespace simpletorrent