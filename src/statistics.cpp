#include "simpletorrent/Statistics.h"

#include <indicators/cursor_control.hpp>

#include "simpletorrent/GlobalState.h"

namespace simpletorrent {

Statistics::Statistics() {
  progress_bar_.set_option(indicators::option::BarWidth{50});
  progress_bar_.set_option(indicators::option::Start{"["});
  progress_bar_.set_option(indicators::option::End{"]"});
  progress_bar_.set_option(indicators::option::PostfixText{"Initializing..."});
  progress_bar_.set_option(
      indicators::option::ForegroundColor{indicators::Color::cyan});
  progress_bar_.set_option(indicators::option::ShowPercentage{true});
  progress_bar_.set_option(indicators::option::FontStyles{
      std::vector<indicators::FontStyle>{indicators::FontStyle::bold}});
}

void Statistics::stop_thread() {
  if (drawer_thread_.joinable()) {
    drawer_thread_.join();
    std::cout << std::endl;
  }
}

void Statistics::start_draw() {
  start_time_ = std::chrono::steady_clock::now();
  drawer_thread_ = std::thread([this]() { this->draw_progress(); });
}

Statistics& Statistics::instance() {
  static Statistics instance;

  return instance;
}

void Statistics::update_num_peers(uint32_t num_peers) {
  connected_peers_ = num_peers;
}

void Statistics::update_piece_completed() { completed_pieces_++; }

void Statistics::init(uint32_t num_pieces, uint32_t piece_length,
                      uint64_t total_length) {
  total_pieces_ = num_pieces;
  piece_length_ = piece_length;
  total_length_ = total_length;
}

void Statistics::draw_progress() {
  indicators::show_console_cursor(false);
  while (!globalstate::is_stop_download()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time_)
            .count();
    size_t total_bytes_downloaded = static_cast<size_t>(piece_length_) *
                                    static_cast<size_t>(completed_pieces_);

    double download_speed = 0;
    if (elapsed_time > 0) {
      download_speed =
          static_cast<double>(total_bytes_downloaded / 1024) / elapsed_time;
    }
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << download_speed;
    std::string download_speed_str = stream.str();

    std::string eta_time = "-";
    if (download_speed > 0) {
      auto remaining_length = total_length_ - total_bytes_downloaded;
      eta_time = format_duration(static_cast<uint64_t>(remaining_length) /
                                 static_cast<uint64_t>(download_speed * 1024));
    }

    progress_bar_.set_option(indicators::option::PostfixText{
        "Peers: " + std::to_string(connected_peers_) +
        " | Speed: " + download_speed_str + " KB/s" + " | ETA: " + eta_time});
    progress_bar_.set_progress(static_cast<float>(completed_pieces_) /
                               static_cast<float>(total_pieces_) * 100.0f);
  }
  // Reset the console color
  std::cout << "\033[0m";
  indicators::show_console_cursor(true);
}

std::string Statistics::format_duration(uint64_t total_seconds) {
  int hours = total_seconds / 3600;
  int minutes = (total_seconds % 3600) / 60;
  int seconds = total_seconds % 60;

  std::stringstream ss;
  if (hours > 23) {
    return ">1 day";
  }
  if (hours > 0) {
    ss << hours << "h ";
  }
  if (minutes > 0) {
    ss << minutes << "m ";
  }
  ss << seconds << "s";

  return ss.str();
}

}  // namespace simpletorrent