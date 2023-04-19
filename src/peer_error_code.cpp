#include "simpletorrent/PeerErrorCode.h"

const char* PeerErrorCategory::name() const noexcept {
  return "PeerErrorCategory";
}

std::string PeerErrorCategory::message(int ev) const {
  switch (static_cast<PeerErrorCode>(ev)) {
    case PeerErrorCode::ParseHandshakeResponseFailure:
      return "Parse Handshake Response Failure";
    case PeerErrorCode::CustomError2:
      return "Custom error 2";
    default:
      return "Unknown error";
  }
}

const PeerErrorCategory& peer_error_category() {
  static PeerErrorCategory instance;
  return instance;
}

std::error_code make_error_code(PeerErrorCode e) {
  return {static_cast<int>(e), peer_error_category()};
}
