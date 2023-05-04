#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Constant.h"

namespace simpletorrent {

struct FileMetadata {
  uint64_t file_length;
  std::vector<std::string> paths;
};

struct TorrentMetadata {
  std::vector<std::string> tracker_url_list;
  std::vector<std::string> piece_hashes;
  uint64_t piece_length;
  uint64_t total_length;
  std::string output_path;
  std::string info_hash;
  std::vector<FileMetadata> files;
};

struct PeerConnInfo {
  const std::string ip;
  const uint16_t port;
};

struct BlockRequest {  // this is sent to the Peer

  BlockRequest(uint32_t piece_index, uint32_t block_offset,
               uint32_t block_length)
      : piece_index(piece_index),
        block_offset(block_offset),
        block_length(block_length) {}

  const uint32_t piece_index;
  const uint32_t block_offset;
  const uint32_t block_length;
};

struct Block {  // this is what the Peer sends us
  const uint32_t piece_index;
  const uint32_t block_offset;
  std::vector<uint8_t>::const_iterator data_begin;
  std::vector<uint8_t>::const_iterator data_end;
};

struct PieceMetadata {  // Used in piece_manager, for checking
                        // of hash and whether it is completed
  const std::string piece_hash;
  const uint32_t current_piece_length;
  const uint32_t num_blocks;
  PieceState state;
};
}  // namespace simpletorrent