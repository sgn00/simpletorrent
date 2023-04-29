#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <sha1.hpp>

#include "simpletorrent/Logger.h"
#include "simpletorrent/PeerManager.h"
#include "simpletorrent/TorrentClient.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

TorrentClient::TorrentClient() { our_id_ = "ABCDEFGHIJ1234567890"; }

void TorrentClient::start_download(const std::string& torrent_file) {
  // 1. Parse torrent file
  TorrentMetadata metadata = parse_torrent_file(torrent_file);

  // 2. Fetch peer info from Tracker
  std::cout << "connecting to tracker" << std::endl;
  Tracker tracker(metadata.announce_url, metadata.info_hash, our_id_);
  tracker.update_peers();

  auto peer_conn_info = tracker.get_peers();
  LOG_INFO("Num peers from tracker: {}", peer_conn_info.size());

  PieceManager piece_manager =
      PieceManager(metadata.piece_hashes, metadata.piece_length,
                   PieceManager::DEFAULT_BLOCK_LENGTH, metadata.total_length,
                   metadata.output_file, Buffer::DEFAULT_BUFFER_SIZE);

  PeerManager peer_manager(piece_manager, std::move(peer_conn_info),
                           metadata.info_hash, our_id_,
                           metadata.piece_hashes.size());
  peer_manager.start();
  std::cout << "total num pieces to download: " << metadata.piece_hashes.size()
            << std::endl;
  std::cout << "finished download" << std::endl;
}

TorrentMetadata TorrentClient::parse_torrent_file(
    const std::string& torrent_file) {
  LOG_INFO("Parsing torrent file at {}", torrent_file);

  try {
    std::ifstream input(torrent_file, std::ios::binary);
    if (!input.is_open()) {
      throw ParseException("Failed to open torrent file");
    }

    auto torrent_data_dict = std::get<bencode::dict>(bencode::decode(input));

    auto announce_url =
        std::get<bencode::string>(torrent_data_dict.at("announce"));
    auto info_dict = std::get<bencode::dict>(torrent_data_dict.at("info"));
    auto pieces = std::get<bencode::string>(info_dict.at("pieces"));
    auto piece_length =
        std::get<bencode::integer>(info_dict.at("piece length"));
    auto total_length = std::get<bencode::integer>(info_dict.at("length"));
    auto output_file = std::get<bencode::string>(info_dict.at("name"));

    // Split the 'pieces' string into a vector of individual piece hashes
    std::vector<std::string> piece_hashes;
    for (size_t i = 0; i < pieces.size(); i += 20) {
      piece_hashes.push_back(pieces.substr(i, 20));
    }

    std::string info_string = bencode::encode(torrent_data_dict.at("info"));
    SHA1 checksum;
    checksum.update(info_string);
    std::string info_hash = checksum.final();
    LOG_INFO("Successfully parsed torrent file at {}", torrent_file);
    LOG_INFO(
        "Torrent metadata: announce: {} | num pieces: {} | piece_length: {} | "
        "total "
        "length: {}",
        announce_url, piece_hashes.size(), piece_length, total_length);
    return TorrentMetadata{announce_url, piece_hashes, piece_length,
                           total_length, output_file,  info_hash};
  } catch (const std::exception& e) {
    LOG_CRITICAL("Failed parsing torrent file at {} | Error: {}", torrent_file,
                 e.what());
    terminate_program();
  }
}

}  // namespace simpletorrent