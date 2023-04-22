#pragma once
#include <iostream>

class Helper2 {
 public:
  ~Helper2() { std::cout << "destroying helper 2!" << std::endl; }
};