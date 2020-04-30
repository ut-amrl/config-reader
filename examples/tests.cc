// Copyright 2020 Kyle Vedder (kvedder@seas.upenn.edu)
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
#include <csignal>
#include <iostream>

#include "config_reader/config_reader.h"

void Check(const bool statement) {
  if (!statement) {
    exit(1);
  }
}

int MyFunction() {
  CONFIG_INT(twelve, "twelve");
  config_reader::ConfigReader reader({"test_config2.lua"});
  return CONFIG_twelve;
}

int main() {
  CONFIG_INT(seven, "seven");
  CONFIG_STRING(str, "str");
  CONFIG_FLOAT(seven_point_five, "seven_point_five");
  config_reader::ConfigReader reader({"test_config.lua"});

  Check(CONFIG_seven == 7);
  Check(CONFIG_str == "str");
  Check(std::abs(CONFIG_seven_point_five - 7.5) < 0.0001f);
  Check(MyFunction() == 12);
  Check(CONFIG_seven == 7);
  Check(CONFIG_str == "str");
  Check(std::abs(CONFIG_seven_point_five - 7.5) < 0.0001f);
  std::cout << "All tests passed!\n";
  return 0;
}
