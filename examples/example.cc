// Copyright 2019 Kyle Vedder (kvedder@seas.upenn.edu)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ========================================================================
#include <iostream>

#include "config_reader/config_reader.h"

int main() {
  CONFIG_INT(test_int, "testInt");
  CONFIG_FLOAT(test_float, "tree.stree.number");
  CONFIG_STRING(test_string, "testString");
  config_reader::ConfigReader reader({"config.lua", "config2.lua"});
  while (true) {
    int local_int = cfg_test_int;
    std::cout << local_int << std::endl;
    if (local_int < 42) {
      std::cout << "It's less than 42!" << std::endl;
    }
    std::cout << cfg_test_float << std::endl;
    std::cout << cfg_test_string << std::endl;
  }
  return 0;
}