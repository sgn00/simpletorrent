#include <mutex>
#include <optional>
#include <set>

namespace simpletorrent {

class PieceManager {
 public:
  PieceManager(const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);

 private:
  std::vector<std::string> piece_hashes_;

  size_t piece_length_;

  size_t total_length_;

  std::string output_file_;

  static constexpr size_t default_block_length = 16384;  // 16 KiB, for example
};

}  // namespace simpletorrent