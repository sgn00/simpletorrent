#pragma once
#include <stdexcept>

class ParseHandshakeException : public std::runtime_error {
 public:
  ParseHandshakeException(const std::string& message)
      : runtime_error(message) {}
};

class StateTransitionException : public std::runtime_error {
 public:
  StateTransitionException(const std::string& message)
      : runtime_error(message) {}
};