#pragma once
#include <iostream>

class Helper3 {
 public:
  ~Helper3() { std::cout << "destroying helper 3 !" << std::endl; }
};