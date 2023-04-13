#include "simpletorrent/PieceManager.h"

namespace simpletorrent {

PieceManager::PieceManager(const std::vector<std::string>& piece_hashes,
                           size_t piece_length, size_t total_length,
                           const std::string& output_file)
    : piece_length_(piece_length),
      total_length_(total_length),
      output_file_(output_file) {
  size_t num_pieces = piece_hashes.size();
  pieces_.reserve(num_pieces);
  size_t last_piece_length = total_length % piece_length;

  for (size_t i = 0; i < num_pieces; ++i) {
    size_t current_piece_length =
        (i == num_pieces - 1 && last_piece_length != 0) ? last_piece_length
                                                        : piece_length;
    size_t num_blocks =
        (current_piece_length + DEFAULT_BLOCK_LENGTH - 1) /
        DEFAULT_BLOCK_LENGTH;  // rounding up for integer division

    pieces_.emplace_back(PieceMetadata{
        piece_hashes[i], std::vector<uint8_t>(num_blocks, false)});
  }

  output_file_stream_.open(output_file,
                           std::ios::binary | std::ios::in | std::ios::out);
}

}  // namespace simpletorrent