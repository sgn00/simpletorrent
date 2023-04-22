#include <cassert>
#include <filesystem>
#include <thread>

#include "sha1.hpp"
#include "simpletorrent/PieceManager.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

PieceManager::PieceManager(const std::vector<std::string>& piece_hashes,
                           size_t piece_length, size_t total_length,
                           const std::string& output_file, uint32_t num_peers)
    : piece_length_(piece_length),
      total_length_(total_length),
      output_file_(output_file),
      buffer_(DEFAULT_BLOCK_LENGTH, piece_length_),
      peer_piece_affinity(num_peers, NO_PIECE),
      num_completed_(0),
      write_queue_(5) {
  size_t num_pieces = piece_hashes.size();
  pieces_.reserve(num_pieces);
  std::cout << "total length: " << total_length << std::endl;
  size_t last_piece_length = total_length % piece_length;
  std::cout << "total num pieces: " << num_pieces << std::endl;
  for (size_t i = 0; i < num_pieces; ++i) {
    pieces_left.insert(i);
    size_t current_piece_length =
        (i == num_pieces - 1 && last_piece_length != 0) ? last_piece_length
                                                        : piece_length;
    size_t num_blocks =
        (current_piece_length + DEFAULT_BLOCK_LENGTH - 1) /
        DEFAULT_BLOCK_LENGTH;  // rounding up for integer division
    // std::cout << "piece len of piece " << i << " is " << current_piece_length
    //          << std::endl;
    pieces_.emplace_back(piece_hashes[i],
                         static_cast<uint32_t>(current_piece_length),
                         static_cast<uint32_t>(num_blocks), NOT_STARTED);
  }

  std::ofstream new_file(output_file, std::ios::binary | std::ios::out);

  // Step 2: Close the ofstream
  new_file.close();
  std::filesystem::resize_file(output_file, total_length_);
  output_file_stream_.open(output_file, std::ios::binary | std::ios::out);

  // spawn writer thread
  writer_thread_ = std::thread([this]() { this->file_writer(); });
}

bool PieceManager::add_block(uint32_t peer_id, const Block& block) {
  // static int count = 0;
  uint32_t piece_index = block.piece_index;
  if (!buffer_.has_piece(piece_index)) {
    return false;
  }

  auto [completed, completed_piece] = buffer_.write_block_to_buffer(block);
  if (completed) {
    if (is_verified_piece(piece_index, completed_piece)) {
      std::cout << std::endl;
      std::cout << "Piece " << piece_index << " verified! writing.. "
                << std::endl;

      save_piece(piece_index, completed_piece);
      assert(piece_index < pieces_.size() && "one");
      pieces_.at(piece_index).state =
          COMPLETED;  // if verified, mark as completed
      num_completed_++;

      pieces_left.erase(piece_index);

      std::cout << "Num completed piece: " << num_completed_ << std::endl;
    } else {
      pieces_.at(piece_index).state = NOT_STARTED;
    }

    remove_piece_from_buffer(
        piece_index);  // regardless if valid or not, remove it from buffer
  }

  return true;
}

void PieceManager::remove_piece_from_buffer(uint32_t piece_index) {
  // remove all affinity
  remove_affinity(piece_index);
  buffer_.remove_piece_from_buffer(piece_index);
}

bool PieceManager::is_verified_piece(uint32_t piece_index,
                                     const std::string& data) const {
  // std::cout << "verifying piece " << piece_index << std::endl;
  assert(piece_index < pieces_.size() && "two");
  const auto& hash = pieces_.at(piece_index).piece_hash;
  SHA1 checksum;
  checksum.update(data);
  std::string piece_hash = checksum.final();
  return hex_decode(piece_hash) == hash;
}

void PieceManager::save_piece(uint32_t piece_index, const std::string& data) {
  size_t file_offset = piece_index * piece_length_;
  write_queue_.enqueue(std::pair{file_offset, std::move(data)});
  // output_file_stream_.seekp(file_offset, std::ios::beg);
  // output_file_stream_.write(data.c_str(), data.size());
}

void PieceManager::update_piece_frequencies(
    const std::vector<uint8_t>& peer_bitfield, uint32_t peer_id) {
  assert(peer_bitfield.size() == pieces_.size());
  peers_bitfield_.insert({peer_id, peer_bitfield});

  // int count = 0;
  // for (int i = 0; i < peer_bitfield.size(); i++) {
  //   if (peer_bitfield.at(i) == HAVE) {
  //     count++;
  //   }
  // }
  // std::cout << "peer has: " << count << " pieces" << std::endl;
  // rarity_manager_.update_piece_frequencies(peer_bitfield, peer_id);
}

std::optional<BlockRequest> PieceManager::select_next_block(uint32_t peer_id) {
  if (!peers_bitfield_.count(peer_id)) {  // if peer not yet registered, ignore
    return std::nullopt;
  }
  //  std::cout << "selecting next block" << std::endl;
  std::optional<uint32_t> chosen_piece;

  // 1. first check for peer piece affinity
  assert(peer_id < peer_piece_affinity.size() && peer_id >= 0);
  if (peer_piece_affinity.at(peer_id) != NO_PIECE) {
    //  std::cout << "peer has no affinity" << std::endl;
    // std::cout << "found affinity.." << std::endl;
    // has a piece assigned to it, assigned piece must be in buffer
    chosen_piece = peer_piece_affinity.at(peer_id);
  }

  // 2. if no peer affinity, we decide depending if buffer is full
  if (!chosen_piece.has_value() && !buffer_.is_full()) {
    // std::cout << "buffer isn't full" << std::endl;
    // std::cout << "no affinity.." << std::endl;
    assert(peers_bitfield_.count(peer_id) == 1);
    const auto& bitfield = peers_bitfield_.at(peer_id);
    assert(bitfield.size() == pieces_.size() && "twelve");
    // find a piece and add it to buffer
    // std::cout << "bitfield size: " << bitfield.size() << std::endl;
    // std::cout << "pieces_ size: " << pieces_.size() << std::endl;
    for (size_t i = 0; i < bitfield.size(); i++) {
      assert(i < pieces_.size() && "three");
      assert(i < bitfield.size() && "nine");
      if (pieces_.at(i).state == NOT_STARTED && bitfield.at(i) == HAVE) {
        chosen_piece = std::optional<uint32_t>(i);
        break;
      }
    }
    // std::cout << "over here" << std::endl;
    if (chosen_piece.has_value()) {
      // add to buffer and set affinity
      //  std::cout << "adding piece to buffer" << std::endl;
      int piece_index = chosen_piece.value();
      assert(piece_index < pieces_.size() && "four");
      buffer_.add_piece_to_buffer(piece_index,
                                  pieces_.at(piece_index).num_blocks,
                                  pieces_.at(piece_index).current_piece_length);
      pieces_.at(piece_index).state = BUFFERED;
      assert(peer_id < peer_piece_affinity.size() && peer_id >= 0);
      peer_piece_affinity[peer_id] = piece_index;
    }
  }

  // std::cout << "stage 3" << std::endl;

  // 3. if stil no chosen piece, find in buffer
  if (!chosen_piece.has_value()) {
    // std::cout << "choose random piece from buffer" << std::endl;
    // choose random piece in buffer to fulfill
    assert(peers_bitfield_.count(peer_id) == 1);
    const auto& bitfield = peers_bitfield_.at(peer_id);
    assert(bitfield.size() == pieces_.size() && "thirteen");
    auto pieces_in_buffer = buffer_.get_pieces_in_buffer();

    if (pieces_in_buffer.size() != 0) {
      int start_index = rng_(pieces_in_buffer.size());
      assert(start_index >= 0 && start_index < pieces_in_buffer.size());
      int index = start_index;
      //   std::cout << "entering loop" << std::endl;
      do {
        int piece_index = pieces_in_buffer.at(index);
        //   std::cout << "piece_index: " << piece_index << std::endl;
        //   std::cout << "bitfield size: " << bitfield.size() << std::endl;
        assert(piece_index >= 0 && piece_index < bitfield.size() && "eleven");
        if (bitfield.at(piece_index) == HAVE) {  // set affinity
          chosen_piece = std::optional<uint32_t>(piece_index);
          assert(peer_piece_affinity.size() > peer_id && peer_id >= 0);
          peer_piece_affinity[peer_id] = piece_index;
          assert(piece_index < pieces_.size() && "five");
          if (pieces_.at(piece_index).state == ALL_REQUESTED) {
            //    std::cout << "ASSIGNED AN ALL REQUESTED PIECE!!!!!!!!!!!"
            //             << std::endl;
            buffer_.clear_all_requested(piece_index);
            pieces_.at(piece_index).state = BUFFERED;
          }

          break;
        }

        index = (index + 1) % pieces_in_buffer.size();
      } while (index != start_index);

      //  std::cout << "exiting loop" << std::endl;
    }
  }

  // std::cout << "over here X" << std::endl;

  if (!chosen_piece.has_value()) {
    std::cout << "could not find piece.." << std::endl;
    return std::nullopt;
  }

  //  std::cout << "retrieving block..." << std::endl;
  uint32_t piece_index = chosen_piece.value();
  auto [all_requested_or_completed, block_idx_to_retrieve] =
      buffer_.get_block_index_to_retrieve(
          piece_index);  // retrieve the block_idx that has not yet been
                         // downloaded

  if (all_requested_or_completed) {
    assert(piece_index < pieces_.size() && "six");
    pieces_.at(piece_index).state = ALL_REQUESTED;
    remove_affinity(piece_index);
  }

  if (!block_idx_to_retrieve.has_value()) {  // should not really occur
    std::cout << "could not find block" << std::endl;
    return std::nullopt;
  }
  // std::cout << "piece to retrieve: " << piece_index << std::endl;
  // std::cout << "block_idx_to_retrieve: " << block_idx_to_retrieve.value()
  //         << std::endl;

  uint32_t block_len = DEFAULT_BLOCK_LENGTH;
  if (piece_index ==
      pieces_.size() - 1) {  // last piece, calculate block len specially
    auto count_len = pieces_.back().current_piece_length -
                     block_idx_to_retrieve.value() * DEFAULT_BLOCK_LENGTH;
    if (count_len >= DEFAULT_BLOCK_LENGTH) {
      block_len = DEFAULT_BLOCK_LENGTH;
    } else {
      block_len = count_len;
      // std::cout << "special block len: " << block_len << std::endl;
    }
  }
  // std::cout << "constructing block request" << std::endl;
  BlockRequest block_request(piece_index, block_idx_to_retrieve.value(),
                             block_len);
  return block_request;
}

bool PieceManager::is_download_complete() const {
  return num_completed_ == pieces_.size();
}

void PieceManager::remove_affinity(uint32_t piece_index) {
  for (size_t i = 0; i < peer_piece_affinity.size(); i++) {
    if (peer_piece_affinity.at(i) == piece_index) {
      peer_piece_affinity.at(i) = NO_PIECE;
    }
  }
}

void PieceManager::file_writer() {
  uint32_t write_count = 0;
  while (!is_download_complete() || write_count != pieces_.size()) {
    // std::cout << "write count: " << write_count << " pieces: " <<
    // pieces_.size()
    //           << std::endl;
    std::pair<size_t, std::string> value;
    if (write_queue_.try_dequeue(value)) {
      size_t file_offset = value.first;
      output_file_stream_.seekp(file_offset, std::ios::beg);
      output_file_stream_.write(value.second.c_str(), value.second.size());
      write_count++;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  std::cout << "trying to close output file stream" << std::endl;
  output_file_stream_.close();
}

PieceManager::~PieceManager() {
  std::cout << "starting destroying piece manager" << std::endl;

  std::cout << "unfulfilled num pieces: " << pieces_left.size() << std::endl;
  for (auto i : pieces_left) {
    assert(i < pieces_.size() && "seven");
    std::cout << "Piece num: " << i << " state: " << +pieces_.at(i).state
              << std::endl;
  }
  auto pieces_in_buffer = buffer_.get_pieces_in_buffer();
  for (auto i : pieces_in_buffer) {
    std::cout << "piece " << i << " is in buffer" << std::endl;
  }

  std::cout << "checking peer bit field" << std::endl;
  for (auto& [k, v] : peers_bitfield_) {
    std::cout << "pb: " << k << ", size: " << v.size() << std::endl;
  }

  writer_thread_.join();
  std::cout << "destroying piece manager" << std::endl;
}

}  // namespace simpletorrent