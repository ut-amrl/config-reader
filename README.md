# Config Reader

A strongly typed, header only Lua-based config file reader for C++ 11. Supports runtime reloading of parameter values on config file save.

# Example Usage

```C++
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
```