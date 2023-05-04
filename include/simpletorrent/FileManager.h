#pragma once
#include <readerwriterqueue.h>

#include <filesystem>
#include <memory>
#include <thread>

#include "Metadata.h"

namespace simpletorrent {
class FileManager {
 public:
  FileManager(const TorrentMetadata& data);

  void save_piece(size_t file_offset, const std::string& data);

  ~FileManager();

 private:
  uint32_t num_pieces_;
  std::vector<std::shared_ptr<std::ofstream>> open_files_;
  std::vector<size_t> file_lengths_;
  moodycamel::ReaderWriterQueue<std::pair<uint32_t, std::string>> write_queue_;
  std::thread writer_thread_;

  std::vector<std::tuple<std::size_t, std::size_t, std::size_t>>
  calculate_files_write_info(std::size_t offset, std::size_t piece_length);

  std::shared_ptr<std::ofstream> create_and_open_file(
      const std::filesystem::path& path, size_t total_length);

  void file_writer();
};
}  // namespace simpletorrent