#pragma once
#include <cstdint>
#include <string_view>

using namespace std::literals;

namespace simpletorrent {

constexpr auto protocol_identifier = "BitTorrent protocol"sv;

// const uint8_t HAVE = 1;
// const uint8_t DONT_HAVE = 0;

// const uint8_t REQUESTED = 4;

// const uint8_t ALL_REQUESTED = 3;

// const int NO_PIECE = -1;

// const uint8_t NOT_STARTED = 0;
// const uint8_t COMPLETED = 1;
// const uint8_t BUFFERED = 2;

enum class PieceState { NOT_STARTED, ALL_REQUESTED, BUFFERED, COMPLETED };

enum class PeerPieceState { HAVE, DONT_HAVE };

enum class BlockState { HAVE, DONT_HAVE, REQUESTED };

}  // namespace simpletorrent