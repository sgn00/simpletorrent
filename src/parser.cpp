#include "simpletorrent/Parser.h"

#include <fstream>
#include <iostream>
#include <sha1.hpp>

#include "simpletorrent/Logger.h"
#include "simpletorrent/Util.h"

namespace simpletorrent {

TorrentMetadata Parser::parse_torrent_file(const std::string& torrent_file) {
  LOG_INFO("Parsing torrent file at {}", torrent_file);

  try {
    std::ifstream input(torrent_file, std::ios::binary);
    if (!input.is_open()) {
      throw ParseException("Failed to open torrent file");
    }

    auto torrent_data_dict = std::get<bencode::dict>(bencode::decode(input));

    TorrentMetadata data;

    auto info_dict = std::get<bencode::dict>(torrent_data_dict.at("info"));

    bool is_single_file = info_dict.count("length") == 1;

    data.piece_hashes = parse_piece_hash(info_dict);

    data.announce_url =
        std::get<bencode::string>(torrent_data_dict.at("announce"));

    data.output_path = std::get<bencode::string>(info_dict.at("name"));

    data.info_hash = calc_info_hash(torrent_data_dict);

    data.piece_length =
        std::get<bencode::integer>(info_dict.at("piece length"));

    if (is_single_file) {
      data.total_length = std::get<bencode::integer>(info_dict.at("length"));
    } else {
      data.total_length = 0;
      auto file_list = std::get<bencode::list>(info_dict.at("files"));
      std::vector<FileMetadata> files;
      for (const auto& f : file_list) {
        FileMetadata file;
        auto f_dict = std::get<bencode::dict>(f);
        file.file_length = std::get<bencode::integer>(f_dict.at("length"));
        data.total_length += file.file_length;  // add to total length
        std::vector<std::string> paths{
            data.output_path};  // insert root folder name
        auto p_list = std::get<bencode::list>(f_dict.at("path"));
        for (const auto& p : p_list) {
          paths.push_back(std::get<bencode::string>(p));
        }
        file.paths = paths;
        files.push_back(file);
      }
      data.files = files;
    }

    LOG_INFO("Successfully parsed torrent file at {}", torrent_file);
    LOG_INFO(
        "Torrent metadata: announce: {} | num pieces: {} | piece_length: "
        "{} | "
        "total "
        "length: {} | Is multi file: {}",
        data.announce_url, data.piece_hashes.size(), data.piece_length,
        data.total_length, !data.files.empty());

    // std::cout << "Torrent Metadata" << std::endl;
    // std::cout << "Announce url: " << data.announce_url << std::endl;
    // std::cout << "Piece hashes size: " << data.piece_hashes.size() <<
    // std::endl; std::cout << "Piece len: " << data.piece_length << std::endl;
    // std::cout << "Total len: " << data.total_length << std::endl;
    // std::cout << "Info hash: " << data.info_hash << std::endl;
    // std::cout << "Output path: " << data.output_path << std::endl;
    // std::cout << "Files: " << std::endl;
    // for (const auto& f : data.files) {
    //   std::cout << "  "
    //             << "Len: " << f.file_length << " ";
    //   for (const auto& p : f.paths) {
    //     std::cout << p << "|";
    //   }
    //   std::cout << std::endl;
    // }

    return data;
  } catch (const std::exception& e) {
    LOG_CRITICAL("Failed parsing torrent file at {} | Error: {}", torrent_file,
                 e.what());
    terminate_program();
  }
}

std::vector<std::string> Parser::parse_piece_hash(
    const bencode::dict& info_dict) {
  auto pieces = std::get<bencode::string>(info_dict.at("pieces"));
  std::vector<std::string> piece_hashes;
  for (size_t i = 0; i < pieces.size(); i += 20) {
    piece_hashes.push_back(pieces.substr(i, 20));
  }
  return piece_hashes;
}

std::string Parser::calc_info_hash(const bencode::dict& data_dict) {
  std::string info_string = bencode::encode(data_dict.at("info"));
  SHA1 checksum;
  checksum.update(info_string);
  return checksum.final();
}

}  // namespace simpletorrent