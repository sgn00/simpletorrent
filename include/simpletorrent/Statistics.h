#pragma once

#include <chrono>
#include <indicators/block_progress_bar.hpp>
#include <indicators/progress_spinner.hpp>
#include <thread>

namespace simpletorrent {
class Statistics {
 public:
  static Statistics& instance();

  // Disable copy constructor and assignment operator
  Statistics(const Statistics&) = delete;
  Statistics& operator=(const Statistics&) = delete;

  void update_num_peers(uint32_t num_peers);

  void update_piece_completed();

  void init(uint32_t num_pieces, uint32_t piece_length);

  void start_draw();

  void stop_thread();

 private:
  uint32_t total_pieces_;
  uint32_t piece_length_;
  uint32_t completed_pieces_;
  uint32_t connected_peers_;
  std::chrono::_V2::steady_clock::time_point start_time_;
  indicators::BlockProgressBar progress_bar_;
  std::thread drawer_thread_;

  Statistics();

  void draw_progress();
};
}  // namespace simpletorrent