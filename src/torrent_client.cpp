#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sha1.hpp>

#include "simpletorrent/Logger.h"
#include "simpletorrent/Parser.h"
#include "simpletorrent/PeerManager.h"
#include "simpletorrent/Statistics.h"
#include "simpletorrent/TorrentClient.h"
#include "simpletorrent/Tracker.h"

namespace simpletorrent {

TorrentClient::TorrentClient() : our_id_(generate_random_client_id(20)) {}

std::string TorrentClient::generate_random_client_id(size_t length) {
  constexpr std::string_view alphanum =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0,
                                       static_cast<int>(alphanum.size() - 1));

  std::string result;
  result.reserve(length);
  std::generate_n(std::back_inserter(result), length,
                  [&]() { return alphanum[dist(gen)]; });

  return result;
}

void TorrentClient::start_download(const std::string& torrent_file) {
  // 1. Parse torrent file
  TorrentMetadata metadata = parser::parse_torrent_file(torrent_file);

  std::cout << "Num trackers: " << metadata.tracker_url_list.size()
            << std::endl;

  Tracker tracker(metadata.tracker_url_list, metadata.info_hash, our_id_);

  // 2. Fetch peer info from Tracker
  std::cout << "connecting to trackers" << std::endl;
  tracker.add_peers();

  auto peer_conn_info = tracker.get_peers();
  LOG_INFO("Num peers from trackers: {}", peer_conn_info.size());

  PieceManager piece_manager =
      PieceManager(metadata, PieceManager::DEFAULT_BLOCK_LENGTH,
                   Buffer::DEFAULT_BUFFER_SIZE);

  auto& stats = Statistics::instance();
  stats.init(metadata.piece_hashes.size(), metadata.piece_length);
  stats.start_draw();

  PeerManager peer_manager(piece_manager, std::move(peer_conn_info),
                           metadata.info_hash, our_id_,
                           metadata.piece_hashes.size());
  peer_manager.start();
  // std::cout << "total num pieces to download: " <<
  // metadata.piece_hashes.size()
  //           << std::endl;
  // std::cout << "finished download" << std::endl;
}

}  // namespace simpletorrent