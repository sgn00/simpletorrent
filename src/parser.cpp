#include "simpletorrent/Parser.h"

#include <fstream>
#include <iostream>
#include <sha1.hpp>

#include "simpletorrent/Logger.h"

namespace simpletorrent {

namespace parser {

namespace {
std::vector<std::string> parse_piece_hash(const bencode::dict& info_dict) {
  auto pieces = std::get<bencode::string>(info_dict.at("pieces"));
  std::vector<std::string> piece_hashes;
  for (size_t i = 0; i < pieces.size(); i += 20) {
    piece_hashes.push_back(pieces.substr(i, 20));
  }
  return piece_hashes;
}

std::string calc_info_hash(const bencode::dict& data_dict) {
  std::string info_string = bencode::encode(data_dict.at("info"));
  SHA1 checksum;
  checksum.update(info_string);
  return checksum.final();
}

void add_trackers(const bencode::dict& torrent_data_dict,
                  TorrentMetadata& data) {
  // if announce list not empty, get trackers from announce-list
  if (torrent_data_dict.count("announce-list")) {
    auto announce_list =
        std::get<bencode::list>(torrent_data_dict.at("announce-list"));
    for (const auto& list_data : announce_list) {
      auto list = std::get<bencode::list>(list_data);
      for (const auto& str_data : list) {
        auto tracker_url = std::get<bencode::string>(str_data);
        data.tracker_url_list.push_back(tracker_url);
      }
    }
  } else {  // else take the single tracker in announce
    auto announce_url =
        std::get<bencode::string>(torrent_data_dict.at("announce"));
    data.tracker_url_list.push_back(announce_url);
  }
}

void add_files(const bencode::dict& info_dict, TorrentMetadata& data) {
  bool is_single_file = info_dict.count("length") == 1;

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
}

}  // namespace

TorrentMetadata parse_torrent_file(const std::string& torrent_file) {
  LOG_INFO("Parsing torrent file at {}", torrent_file);

  try {
    std::ifstream input(torrent_file, std::ios::binary);
    if (!input.is_open()) {
      throw ParseException("failed to open torrent file");
    }

    LOG_INFO("Reading torrent file: {}", torrent_file);
    std::cout << "Reading torrent file: " << torrent_file << std::endl;

    auto torrent_data_dict = std::get<bencode::dict>(bencode::decode(input));

    TorrentMetadata data;

    auto info_dict = std::get<bencode::dict>(torrent_data_dict.at("info"));

    data.piece_hashes = parse_piece_hash(info_dict);

    add_trackers(torrent_data_dict, data);

    if (data.tracker_url_list.empty()) {
      throw ParseException("no HTTP trackers found");
    }

    data.output_path = std::get<bencode::string>(info_dict.at("name"));

    data.info_hash = calc_info_hash(torrent_data_dict);

    data.piece_length =
        std::get<bencode::integer>(info_dict.at("piece length"));

    add_files(info_dict, data);

    LOG_INFO("Successfully parsed torrent file at {}", torrent_file);
    LOG_INFO(
        "Torrent metadata: num trackers {} | num pieces: {} | piece_length: "
        "{} | "
        "total "
        "length: {} | Is multi file: {}",
        data.tracker_url_list.size(), data.piece_hashes.size(),
        data.piece_length, data.total_length, !data.files.empty());

    return data;
  } catch (const std::exception& e) {
    LOG_CRITICAL("Failed parsing torrent file at {} | Error: {}", torrent_file,
                 e.what());
    throw ParseException("failed parsing torrent file at " + torrent_file +
                         " | Error: " + e.what());
  }
}

}  // namespace parser

}  // namespace simpletorrent