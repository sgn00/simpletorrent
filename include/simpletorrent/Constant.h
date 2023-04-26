#pragma once
#include <cstdint>
#include <string_view>

using namespace std::literals;

namespace simpletorrent {

constexpr auto protocol_identifier = "BitTorrent protocol"sv;

enum class PieceState { NOT_STARTED, ALL_REQUESTED, BUFFERED, COMPLETED };

enum class PeerPieceState { HAVE, DONT_HAVE };

enum class BlockState { HAVE, DONT_HAVE, REQUESTED };

enum class PeerState {
  NOT_CONNECTED,
  CONNECTED_1,
  CONNECTED_2,
  DISCONNECTED_1,
  DISCONNECTED_2
};

}  // namespace simpletorrent