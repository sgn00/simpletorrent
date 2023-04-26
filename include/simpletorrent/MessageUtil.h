#pragma once
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

#include "Constant.h"
#include "Metadata.h"
#include "Util.h"

namespace simpletorrent {
namespace message_util {

enum class MessageType : uint8_t {
  Choke = 0,
  Unchoke = 1,
  Interested = 2,
  NotInterested = 3,
  Have = 4,
  Bitfield = 5,
  Request = 6,
  Piece = 7,
  Cancel = 8,
  Port = 9
};

inline std::optional<std::string> parse_handshake_response(
    const std::array<uint8_t, 68>& handshake_response,
    const std::string& info_hash) {
  size_t idx = 0;

  // Extract the protocol name length (first byte)
  uint8_t protocol_name_length = handshake_response[idx++];

  if (idx + protocol_name_length > handshake_response.size()) {
    return std::nullopt;
  }

  // Extract the protocol name
  std::string protocol_name(
      handshake_response.begin() + idx,
      handshake_response.begin() + idx + protocol_name_length);
  idx += protocol_name_length;

  if (protocol_name != simpletorrent::protocol_identifier) {
    return std::nullopt;
  }

  // Extract the reserved bytes
  std::array<uint8_t, 8> reserved_bytes;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 8, reserved_bytes.begin());
  idx += 8;

  // Extract the info_hash
  std::array<uint8_t, 20> binary_info_hash;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 20, binary_info_hash.begin());
  idx += 20;

  std::string hex_info_hash = to_hex_string(binary_info_hash);

  if (hex_info_hash != info_hash) {
    return std::nullopt;
  }

  // Extract the peer_id
  std::array<uint8_t, 20> binary_peer_id;
  std::copy(handshake_response.begin() + idx,
            handshake_response.begin() + idx + 20, binary_peer_id.begin());

  return to_hex_string(binary_peer_id);
}

inline std::vector<uint8_t> create_handshake(
    const std::array<uint8_t, 8>& reserved_bytes,
    const std::string& binary_info_hash, const std::string& our_id) {
  std::vector<uint8_t> handshake;
  handshake.push_back(static_cast<uint8_t>(protocol_identifier.size()));

  // 2. Protocol identifier
  handshake.insert(handshake.end(), protocol_identifier.begin(),
                   protocol_identifier.end());

  // 3. 8 reserved bytes
  handshake.insert(handshake.end(), reserved_bytes.begin(),
                   reserved_bytes.end());

  // 4. Info_hash
  handshake.insert(handshake.end(), binary_info_hash.begin(),
                   binary_info_hash.end());

  // 5. Peer_id
  handshake.insert(handshake.end(), our_id.begin(), our_id.end());

  return handshake;
}

inline std::vector<uint8_t> construct_message(
    MessageType type, const std::vector<uint8_t>& payload = {}) {
  uint32_t length = static_cast<uint32_t>(payload.size() + 1);
  std::vector<uint8_t> message;

  // Length
  for (int i = 3; i >= 0; --i) {
    message.push_back((length >> (8 * i)) & 0xFF);
  }

  // Message type
  message.push_back(static_cast<uint8_t>(type));

  // Payload
  message.insert(message.end(), payload.begin(), payload.end());
  // for (auto& b : message) {
  //   std::cout << b << ",";
  // }
  return message;
}

inline std::vector<uint8_t> construct_request_message(
    const BlockRequest& request) {
  std::vector<uint8_t> payload(12);

  // Set the piece index
  payload[0] = (request.piece_index >> 24) & 0xFF;
  payload[1] = (request.piece_index >> 16) & 0xFF;
  payload[2] = (request.piece_index >> 8) & 0xFF;
  payload[3] = request.piece_index & 0xFF;

  // Set the block offset
  payload[4] = (request.block_offset >> 24) & 0xFF;
  payload[5] = (request.block_offset >> 16) & 0xFF;
  payload[6] = (request.block_offset >> 8) & 0xFF;
  payload[7] = request.block_offset & 0xFF;

  // Set the block length
  payload[8] = (request.block_length >> 24) & 0xFF;
  payload[9] = (request.block_length >> 16) & 0xFF;
  payload[10] = (request.block_length >> 8) & 0xFF;
  payload[11] = request.block_length & 0xFF;

  // Construct the request message using the helper function
  return construct_message(MessageType::Request, payload);
}

inline uint32_t get_header_length(const std::vector<uint8_t>& header) {
  return (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
}

inline std::pair<uint32_t, uint32_t> retrieve_piece_index_and_block_offset(
    const std::vector<uint8_t>& payload) {
  uint32_t piece_index =
      (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];

  uint32_t block_offset =
      (payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7];

  return {piece_index, block_offset};
}

inline std::vector<uint8_t> get_peer_bitfield(
    const std::vector<uint8_t>& payload, uint32_t num_pieces) {
  // Initialize a vector to represent the availability of pieces
  std::vector<uint8_t> peer_bitfield(num_pieces);

  // Parse the bitfield payload
  for (uint32_t i = 0; i < payload.size(); ++i) {
    for (uint8_t j = 0; j < 8; ++j) {
      bool has_piece = (payload[i] & (0x80 >> j)) != 0;
      uint32_t piece_index = i * 8 + j;

      if (piece_index < num_pieces) {
        peer_bitfield[piece_index] = has_piece;
      }
    }
  }
  return peer_bitfield;
}

}  // namespace message_util
}  // namespace simpletorrent