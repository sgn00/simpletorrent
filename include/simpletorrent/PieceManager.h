#include <fstream>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace simpletorrent {

struct BlockRequest {
  int piece_index;
  int block_offset;
  int block_length;
};

struct Block {
  int piece_index;
  int block_offset;
  std::vector<char> data;
};

struct BufferPiece {
  int piece_index;
  std::vector<char> data;
  bool empty = true;
};

struct PieceMetadata {
  std::string piece_hash;
  std::vector<uint8_t> blocks_downloaded;
};

class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);

  std::optional<BlockRequest> request_block(int peer_id);

  bool add_block(int peer_id, const Block& block);

  bool verify_piece(int piece_index);

  void save_piece(int piece_index);

  bool is_download_complete() const;

  std::vector<uint8_t> get_bitfield() const;

 private:
  static constexpr size_t DEFAULT_BLOCK_LENGTH = 16384;  // 16 KiB

  static constexpr size_t DEFAULT_BUFFER_SIZE =
      64;  // max 64 piece parts in memory

  size_t piece_length_;

  size_t total_length_;

  std::string output_file_;

  std::ofstream output_file_stream_;

  std::mutex pieces_mutex_;

  std::vector<PieceMetadata> pieces_;

  std::set<int> rarest_pieces_;

  std::array<BufferPiece, DEFAULT_BUFFER_SIZE> buffer_;

  std::unordered_map<int, int>
      piece_buffer_map_;  // maps piece index to buffer index

  void update_rarest_pieces(const std::vector<uint8_t>& peer_bitfield);

  std::optional<BlockRequest> select_next_block(int peer_id);
};

}  // namespace simpletorrent