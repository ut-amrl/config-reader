// Copyright 2019 Kyle Vedder (kvedder@seas.upenn.edu), 2018 Ishan Khatri
// (ikhatri@umass.edu)
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
#ifndef CONFIGREADER_CONFIG_READER_H_
#define CONFIGREADER_CONFIG_READER_H_

#include <eigen3/Eigen/Core>

extern "C" {
#include <libgen.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <unistd.h>
}

#include <algorithm>
#include <atomic>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "config_reader/lua_script.h"
#include "types/type_interface.h"

#include "types/config_generic.h"
#include "types/config_numeric.h"

namespace config_reader {
// Define constants
static const std::string kDefaultFileName = "config.lua";

#define MAKE_NAME(name) cfg_##name

// Define macros for creating new config vars

#define CONFIG_INT(name, key)                                                  \
  const int& MAKE_NAME(name) =                                                 \
      ::config_reader::InitVar<int, ::config_reader::config_types::ConfigInt>( \
          key)
#define CONFIG_UINT(name, key)                                    \
  const unsigned int& MAKE_NAME(name) = ::config_reader::InitVar< \
      unsigned int, ::config_reader::config_types::ConfigUnsignedInt>(key)
#define CONFIG_DOUBLE(name, key)                            \
  const double& MAKE_NAME(name) = ::config_reader::InitVar< \
      double, ::config_reader::config_types::ConfigDouble>(key)
#define CONFIG_FLOAT(name, key)                            \
  const float& MAKE_NAME(name) = ::config_reader::InitVar< \
      float, ::config_reader::config_types::ConfigFloat>(key)
#define CONFIG_STRING(name, key)                                 \
  const std::string& MAKE_NAME(name) = ::config_reader::InitVar< \
      std::string, ::config_reader::config_types::ConfigString>(key)
#define CONFIG_BOOL(name, key)       \
  const bool& MAKE_NAME(name) =      \
      ::config_reader::InitVar<bool, \
                               ::config_reader::config_types::ConfigBool>(key)

class MapSingleton {
  // Needs large number of buckets because references are not stable across
  // map growths.
  static constexpr int kNumMapBuckets = 1000000;

 public:
  using KeyLookupMap =
      std::unordered_map<std::string,
                         std::unique_ptr<config_types::TypeInterface>>;

  static KeyLookupMap& Singleton() {
    static KeyLookupMap config(kNumMapBuckets);
    return config;
  }

  static std::atomic_bool* NewKeyAdded() {
    static std::atomic_bool new_key_added(false);
    return &new_key_added;
  }
};

template <typename CPPType, typename ConfigType>
const CPPType& InitVar(const std::string& key) {
  ConfigType* t = new ConfigType(key);
  MapSingleton::Singleton()[key] =
      std::unique_ptr<config_types::TypeInterface>(t);
  *MapSingleton::NewKeyAdded() = true;
  return t->GetValue();
}

void LuaRead(const std::vector<std::string>& files) {
  // Create the LuaScript object
  const LuaScript script(files);
  // Loop through the unordered map
  for (const auto& pair : MapSingleton::Singleton()) {
    config_types::TypeInterface* t = pair.second.get();
    if (t->GetType() == config_types::CNULL) {
      std::cerr << "Key has a type CNULL!" << std::endl;
      return;
    }
    t->SetValue(script);
  }
  *MapSingleton::NewKeyAdded() = false;
}

class ConfigReader {
  std::atomic_bool is_running_;
  std::thread daemon_;

  void InitDaemon(const std::vector<std::string>& files) {
    static constexpr int kEventSize = sizeof(inotify_event);
    static constexpr int kEventBufferLength = (1024 * (kEventSize + 16));
    static constexpr int kInotifySleep = 50;
    std::array<char, kEventBufferLength> buffer;

    // Initialize inotify
    int fd = inotify_init();
    if (fd < 0) {
      std::cerr << "ERROR: Couldn't initialize inotify" << std::endl;
      exit(-1);
    }

    // Each watch descriptor is associated with a directory that contains a set
    // of watched files
    std::unordered_map<int, std::set<std::string>> wd_to_files;

    // Add a listener on each parent directory
    for (const std::string& filepath : files) {
      std::string filename = basename(strdup(filepath.c_str()));
      std::string directory = dirname(strdup(filepath.c_str()));

      // (Will be duplicate wd if we've already add_watched the directory)
      int wd = inotify_add_watch(fd, directory.c_str(), IN_MODIFY);

      if (wd == -1) {
        std::cerr << "ERROR: Couldn't add watch to the file: " << filepath
                  << std::endl;
        return;
      }

      // Add to list of watched files
      wd_to_files[wd].insert(filename);
    }

    int epfd = epoll_create(1);
    if (epfd < 0) {
      std::cerr << "ERROR: Call to epoll_create failed." << std::endl;
    }

    epoll_event ready_to_read = {0};
    ready_to_read.data.fd = fd;
    ready_to_read.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ready_to_read)) {
      std::cerr << "ERROR: Call to epoll_ctl failed." << std::endl;
    }

    epoll_event epoll_events;

    auto last_notify = std::chrono::system_clock::now();
    bool needs_update = false;

    // Loop forever, checking for changes to the files above
    while (is_running_) {
      // Wait for 50 ms for there to be an available inotify event
      int nr_events = epoll_wait(epfd, &epoll_events, 1, kInotifySleep);

      // Handle addition of keys after Daemon starts.
      if (*MapSingleton::NewKeyAdded()) {
        LuaRead(files);
        needs_update = false;
        continue;
      }

      if (nr_events < 0) {
        // If the call to epoll_wait failed
        std::cerr << "ERROR: Call to epoll_wait failed." << std::endl;
      }

      if (nr_events > 0) {
        // Else if the inotify fd has recieved something that can be read
        int length = read(fd, &buffer, kEventBufferLength);
        if (length < 0 || length > kEventBufferLength) {
          std::cerr << "ERROR: Inotify read failed" << std::endl;
          continue;
        }

        for (int i = 0; i < length;) {
          inotify_event* event = reinterpret_cast<inotify_event*>(&buffer[i]);
          i += kEventSize + event->len;

          // Length of file name must be positive
          if (event->len <= 0) {
            continue;
          }

          auto res = wd_to_files.find(event->wd);
          if (res == wd_to_files.end()) {
            std::cerr << "Unknown watch descriptor!" << std::endl;
            continue;
          }

          // If file is in our list of files to track for this wd, then update
          if (res->second.count(event->name) != 0) {
            last_notify = std::chrono::system_clock::now();
            needs_update = true;
          }
        }
      }
      if (needs_update && std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now() - last_notify)
                                  .count() > 2 * kInotifySleep) {
        LuaRead(files);
        needs_update = false;
      }
    }

    // Clean up
    close(epfd);
    close(fd);
  }
  void CreateDaemon(const std::vector<std::string>& files) {
    LuaRead(files);
    is_running_ = true;
    daemon_ = std::thread(&ConfigReader::InitDaemon, this, files);
  }

  void Stop() {
    is_running_ = false;
    if (daemon_.joinable()) {
      daemon_.join();
    }
  }

 public:
  ConfigReader() = delete;
  ConfigReader(const std::vector<std::string>& files) { CreateDaemon(files); }
  ~ConfigReader() { Stop(); }
};

}  // namespace config_reader

#endif  // CONFIGREADER_CONFIG_READER_H_