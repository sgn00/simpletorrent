#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <iostream>
#include <sstream>

#include "simpletorrent/GlobalState.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/Peer.h"
#include "simpletorrent/Statistics.h"
#include "simpletorrent/TorrentClient.h"

using namespace simpletorrent;
using namespace indicators;

int main() {
  std::string torrent_file = "../stare.torrent";

  std::string filename = torrent_file;
  auto pos = torrent_file.find_last_of('/');
  if (pos != std::string::npos) {
    filename = torrent_file.substr(pos + 1);
  }

  Logger::initialize(filename + ".log");
  LOG_INFO("Initialized");

  TorrentClient tc;
  try {
    tc.start_download(torrent_file);
    LOG_INFO("Completed");
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    LOG_CRITICAL("FATAL error shutting down | Error: {}", e.what());
  }

  GlobalState::set_stop_download();
  Statistics::instance().stop_thread();

  LOG_INFO("Stopping torrent client...");
  spdlog::shutdown();

  // asio::io_context io_context;
  // PieceManager pm({}, 100, 1000, "abc");

  /*
  const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);
  */

  // Peer p(pm, io_context, "abc", "abc", "127.0.0.1", 8080, 1);
  // Peer p2(pm, io_context, "abc", "abc", "52.217.95.205", 80);
  // asio::co_spawn(
  //   io_context, [&] { return p.start(); }, asio::detached);
  // asio::co_spawn(
  //     io_context, [&] { return p2.start(); }, asio::detached);
  // io_context.run();
  // std::string torrent_file = "../debian-iso.torrent";

  // TorrentClient torrentClient;
  // torrentClient.start_download(torrent_file);
}