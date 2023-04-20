#pragma once

class FastRandom {
 public:
  FastRandom() : a(1664525), c(1013904223), m(1ULL << 32), state(12345) {}

  int operator()(unsigned long max) {
    state = (a * state + c) % m;
    auto res = state % max;
    return static_cast<int>(res);
  }

 private:
  unsigned long a, c, m, state;
};