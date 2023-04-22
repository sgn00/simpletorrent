#pragma once
#include <iostream>

class Helper {
 public:
  ~Helper() { std::cout << "destroying helper in piece manager" << std::endl; }
};