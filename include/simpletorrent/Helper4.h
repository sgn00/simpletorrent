#pragma once
#include <iostream>

class Helper4 {
 public:
  ~Helper4() { std::cout << "destroying helper 4" << std::endl; }
};