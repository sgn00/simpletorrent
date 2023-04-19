#pragma once

#include <system_error>

class PeerErrorCategory : public std::error_category {
 public:
  const char* name() const noexcept override;
  std::string message(int ev) const override;
};

enum class PeerErrorCode {
  ParseHandshakeResponseFailure = 1,
  CustomError2 = 2,
};

const PeerErrorCategory& peer_error_category();
std::error_code make_error_code(PeerErrorCode e);

namespace std {
template <>
struct is_error_code_enum<PeerErrorCode> : true_type {};
}  // namespace std
