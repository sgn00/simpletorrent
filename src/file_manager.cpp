#include <fstream>
#include <iostream>

#include "simpletorrent/FileManager.h"
#include "simpletorrent/GlobalState.h"
#include "simpletorrent/Logger.h"

namespace simpletorrent {
FileManager::FileManager(const TorrentMetadata& data)
    : num_pieces_(data.piece_hashes.size()), write_queue_(5) {
  if (data.files.empty()) {  // single file torrent
    open_files_.push_back(
        std::pair(data.total_length,
                  create_and_open_file(data.output_path, data.total_length)));
  } else {  // multi file torrent
    for (const auto& f_metadata : data.files) {
      auto file_length = f_metadata.file_length;
      std::filesystem::path file_path;
      for (const auto& section : f_metadata.paths) {
        file_path /= section;
      }
      open_files_.push_back(
          std::pair(f_metadata.file_length,
                    create_and_open_file(file_path, f_metadata.file_length)));
    }
  }

  writer_thread_ = std::thread([this]() { this->file_writer(); });
}

FileManager::~FileManager() {
  LOG_INFO("FileManager: destroying file manager");
  writer_thread_.join();
  LOG_INFO("FileManager: joined writer thread");
}

void FileManager::save_piece(size_t file_offset, const std::string& data) {
  write_queue_.enqueue(std::pair{file_offset, std::move(data)});
}

std::pair<uint32_t, size_t> FileManager::get_file_and_offset(size_t offset) {
  long long current_offset = 0;
  for (size_t i = 0; i < open_files_.size(); ++i) {
    if (current_offset + open_files_[i].first > offset) {
      return {i, offset - current_offset};
    }
    current_offset += open_files_[i].first;
  }
  // throw exception
}

std::shared_ptr<std::ofstream> FileManager::create_and_open_file(
    const std::filesystem::path& path, size_t total_length) {
  // create any parent directories
  std::cout << "Path: " << path << std::endl;
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  std::ofstream new_file(path, std::ios::binary | std::ios::out);
  new_file.close();
  std::filesystem::resize_file(path, total_length);

  std::ofstream ret_stream;
  ret_stream.open(path, std::ios::binary | std::ios::out);
  return std::make_shared<std::ofstream>(std::move(ret_stream));
}

void FileManager::file_writer() {
  LOG_INFO("FileManager: writer thread spawned");
  uint32_t write_count = 0;
  while (!GlobalState::is_stop_download() && write_count != num_pieces_) {
    std::pair<size_t, std::string> value;
    if (write_queue_.try_dequeue(value)) {
      // depending on single or multifile torrent we do different things
      auto output_file_stream_ptr = open_files_[0].second;
      size_t file_offset = value.first;
      if (open_files_.size() != 1) {  // multi file torrent
        auto [file_index, write_offset] = get_file_and_offset(value.first);
        output_file_stream_ptr = open_files_[file_index].second;
        file_offset = write_offset;
      }

      output_file_stream_ptr->seekp(file_offset, std::ios::beg);
      output_file_stream_ptr->write(value.second.c_str(), value.second.size());
      write_count++;

    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  // std::cout << "stop download: " << stop_download << std::endl;
  // std::cout << "write count: " << write_count
  //           << "pieces size: " << pieces_.size() << std::endl;
}

}  // namespace simpletorrent