#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>

namespace cef_ffmpeg_test {
namespace render {

struct native_window_t;

class window {
public:
  window(const uint32_t width, const uint32_t height);
  ~window() noexcept;

  // для XORG
  void update();

  bool should_close();
  void swap_buffers();

  std::tuple<uint32_t, uint32_t> dims() const;
private:
  std::unique_ptr<native_window_t> wnd;
};

}
}