#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace cef_ffmpeg_test {
namespace common {

enum class format {
  RGBA,
  BGRA,
  YUV420
};

enum class buffering {
  single = 1,
  edouble,
  triple
};

struct slot_t {
  size_t id;
  float x, y, width, height;
};

struct handle_t { size_t id; void* mem; inline bool valid() const { return mem != nullptr; } };

struct frame_t {
  handle_t handle;
  uint32_t width, height;
  enum format format;

  inline bool valid() const { return handle.valid(); }
};

constexpr uint32_t make_color(const float r, const float g, const float b, const float a) {
  return uint32_t(std::clamp(r, 0.0f, 1.0f) * 255u) << 0 |
    uint32_t(std::clamp(g, 0.0f, 1.0f) * 255u) <<   8 |
    uint32_t(std::clamp(b, 0.0f, 1.0f) * 255u) <<  16 |
    uint32_t(std::clamp(a, 0.0f, 1.0f) * 255u) <<  24;
}

extern std::atomic<size_t> g_exit_not_ready;
extern std::atomic<bool> g_closing;

}
}