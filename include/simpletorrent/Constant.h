#pragma once
#include <cstdint>

namespace simpletorrent {
const uint8_t HAVE = 1;
const uint8_t DONT_HAVE = 0;

const uint8_t REQUESTED = 4;

const uint8_t ALL_REQUESTED = 3;

const int NO_PIECE = -1;

const uint8_t NOT_STARTED = 0;
const uint8_t COMPLETED = 1;
const uint8_t BUFFERED = 2;

}  // namespace simpletorrent