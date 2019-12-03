# Config Reader

A strongly typed, header only Lua-based config file reader for C++ 11. Supports runtime reloading of parameter values on config file save.

[![Build Status](https://travis-ci.com/ut-amrl/config-reader.svg?token=rBLDT1qXfkKmkTerGLzY&branch=master)](https://travis-ci.com/ut-amrl/config-reader)

# Example Usage

```C++
#include <csignal>
#include <iostream>

#include "config_reader/config_reader.h"

bool running = true;

void SigIntHandler(int signum) { running = false; }

int main() {
  signal(SIGINT, SigIntHandler);

  CONFIG_INT(test_int, "testInt");
  CONFIG_FLOAT(test_float, "tree.stree.number");
  CONFIG_STRING(test_string, "testString");
  config_reader::ConfigReader reader({"config.lua", "config2.lua"});
  while (running) {
    int local_int = CONFIG_test_int;
    std::cout << local_int << std::endl;
    if (local_int < 42) {
      std::cout << "It's less than 42!" << std::endl;
    }
    std::cout << CONFIG_test_float << std::endl;
    std::cout << CONFIG_test_string << std::endl;
  }
  return 0;
}
```

# Dependencies

 - Lua 5.1 Development Package
 - C++ compiler with C++ 11 support (e.g. `clang++`)

 Ubuntu packages can be installed via `InstallPackages`.

 # Currently Supported C++ Types

  - `int`
  - `unsigned int`
  - `double`
  - `float`
  - `std::string`
  - `bool`
  
