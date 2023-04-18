#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include "simpletorrent/Peer.h"
#include "simpletorrent/TorrentClient.h"

using namespace simpletorrent;

int main() {
  asio::io_context io_context;
  PieceManager pm({}, 100, 1000, "abc");

  /*
  const std::vector<std::string>& piece_hashes,
               size_t piece_length, size_t total_length,
               const std::string& output_file);
  */

  // Peer p(pm, io_context, "abc", "abc", "127.0.0.1", 8080);
  // Peer p2(pm, io_context, "abc", "abc", "52.217.95.205", 80);
  // asio::co_spawn(
  //     io_context, [&] { return p.start(); }, asio::detached);
  // asio::co_spawn(
  //     io_context, [&] { return p2.start(); }, asio::detached);
  // io_context.run();
  // std::string torrent_file = "../debian-iso.torrent";

  // TorrentClient torrentClient;
  // torrentClient.start_download(torrent_file);
}