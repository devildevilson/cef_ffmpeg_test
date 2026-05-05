#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace cef_ffmpeg_test {
namespace common {

struct settings {
  struct source_t {
    std::string name;
    std::string type;
    std::string resource;
    uint32_t width, height;
    float scale;
  };

  struct slot_t {
    std::string name;
    float x, y, width, height;
  };

  struct window_t {
    uint32_t width, height;
  };

  std::vector<source_t> sources;
  std::vector<slot_t> slots;
  struct window_t window;
};

settings parse_settings(std::string path = "../settings.json");

}
}