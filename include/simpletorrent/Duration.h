#pragma once
#include <cstdint>

namespace simpletorrent {
namespace duration {
constexpr uint32_t CONNECT_TIMEOUT = 15;
constexpr uint32_t HANDSHAKE_TIMEOUT = 15;
constexpr uint32_t CHOKE_TIMEOUT = 40;
constexpr uint32_t WAIT_MSG_TIMEOUT = 20;
constexpr uint32_t RECV_MSG_TIMEOUT = 5;
constexpr uint32_t CLEANUP_INTERVAL = 5;
}  // namespace duration
}  // namespace simpletorrent