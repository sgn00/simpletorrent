#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include "simpletorrent/Logger.h"
#include "simpletorrent/Peer.h"
#include "simpletorrent/TorrentClient.h"
#include "simpletorrent/Util.h"

using namespace simpletorrent;

int main() {
  std::string torrent_file = "../debian.torrent";

  Logger::initialize(get_filename_from_path(torrent_file) + ".log");
  LOG_INFO("Initialized");

  TorrentClient tc;
  tc.start_download(torrent_file);

  LOG_INFO("Completed");

  std::cout << "FINISHED EXITING" << std::endl;

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