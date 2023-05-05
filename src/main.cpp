#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <bencode.hpp>
#include <fstream>
#include <indicators/cursor_control.hpp>
#include <iostream>
#include <sstream>

#include "simpletorrent/GlobalState.h"
#include "simpletorrent/Logger.h"
#include "simpletorrent/Peer.h"
#include "simpletorrent/Statistics.h"
#include "simpletorrent/TorrentClient.h"

using namespace simpletorrent;

void sigint_handler(int);
std::string get_filename(const std::string&);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <path to torrent file>" << std::endl;
    return 1;
  }

  signal(SIGINT, sigint_handler);

  std::string torrent_file = argv[1];
  std::string filename = get_filename(torrent_file);

  logger::initialize(filename + ".log");
  LOG_INFO("Initialized");

  TorrentClient tc;
  try {
    tc.start_download(torrent_file);
  } catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
    std::cout << "Terminating simpletorrent..." << std::endl;
    LOG_CRITICAL("FATAL error shutting down | Error: {}", e.what());
  }

  globalstate::set_stop_download();

  LOG_INFO("Stopping torrent client...");
  spdlog::shutdown();
}

void sigint_handler(int signal_num) {
  std::cout << "Stopping torrent client..." << std::endl;
  globalstate::set_stop_download();
  _exit(signal_num);
}

std::string get_filename(const std::string& torrent_file) {
  auto pos = torrent_file.find_last_of('/');
  if (pos != std::string::npos) {
    return torrent_file.substr(pos + 1);
  } else {
    return torrent_file;
  }
}