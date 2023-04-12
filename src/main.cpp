#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

#include "simpletorrent/TorrentClient.h"

using namespace simpletorrent;

int main() {
  std::string torrent_file = "../debian-iso.torrent";

  TorrentClient torrentClient;
  torrentClient.start_download(torrent_file);
}