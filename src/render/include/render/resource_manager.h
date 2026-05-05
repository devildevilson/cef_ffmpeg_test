#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <vector>
#include <array>

#include "common/common.h"


namespace cef_ffmpeg_test {
namespace render {

class system;

/**
* resource_manager выступает thread safe посредником между источниками и рендером
*/

class resource_manager {
public:
  resource_manager() noexcept;

  size_t register_frame(system* sys, const uint32_t width, const uint32_t height, const enum common::format format, const enum common::buffering buffering);
  void register_slot(const size_t id, const float x, const float y, const float w, const float h);

  common::frame_t acquire_write_frame(const size_t id) const;
  void swap_write_frame(const size_t id);

  common::frame_t acquire_read_frame(const size_t id) const;
  void swap_read_frame(const size_t id);

  size_t frames_size() const;
  const std::vector<common::slot_t>& slots() const;
private:
  struct frame_container_t {
    uint32_t width, height;
    enum common::format format;
    enum common::buffering buffering;

    std::atomic<size_t> read;
    std::atomic<size_t> write;

    std::array<common::frame_t, 8> frames;

    frame_container_t() noexcept;

    frame_container_t(const frame_container_t& copy) noexcept;
    frame_container_t(frame_container_t&& move) noexcept;
    frame_container_t& operator=(const frame_container_t&copy) noexcept;
    frame_container_t& operator=(frame_container_t&& move) noexcept;
  };

  std::vector<frame_container_t> resources;

  std::vector<common::slot_t> slots_arr;
};

}
}