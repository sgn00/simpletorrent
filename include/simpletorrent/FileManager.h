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

  std::vector<std::pair<size_t, std::shared_ptr<std::ofstream>>> open_files_;

  moodycamel::ReaderWriterQueue<std::pair<uint32_t, std::string>> write_queue_;

  std::pair<uint32_t, size_t> get_file_and_offset(size_t offset);

  std::shared_ptr<std::ofstream> create_and_open_file(
      const std::filesystem::path& path, size_t total_length);

  void file_writer();

  std::thread writer_thread_;
};
}  // namespace simpletorrent