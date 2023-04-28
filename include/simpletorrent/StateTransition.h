#pragma once
#include "Constant.h"
#include "CustomException.h"

namespace simpletorrent {

inline PeerState get_next_peer_state(PeerState state) {
  if (state == PeerState::NOT_CONNECTED) {
    return PeerState::CONNECTED_1;
  } else if (state == PeerState::CONNECTED_1) {
    return PeerState::DISCONNECTED_1;
  } else if (state == PeerState::DISCONNECTED_1) {
    return PeerState::CONNECTED_2;
  } else if (state == PeerState::CONNECTED_2) {
    return PeerState::DISCONNECTED_2;
  } else {
    throw StateTransitionException(
        "Invalid state or no state to transition to");
  }
}
}  // namespace simpletorrent