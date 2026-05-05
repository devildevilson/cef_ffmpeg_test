#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <vector>

#include "common/source_interface.h"

namespace cef_ffmpeg_test {
namespace render {
class resource_manager;
}

namespace source {

struct ffmpeg_frame_data {
  uint8_t* pixels;
  int32_t stride;
  int64_t timestamp; // таймстемпа походу может быть какой попало
};

static_assert(std::is_pod_v<ffmpeg_frame_data>);

struct ffmpeg_context;

class ffmpeg : public common::source_interface {
public:
  ffmpeg(std::string path, const size_t queue_size = 5);
  ~ffmpeg() noexcept;

  void set_update_manager(render::resource_manager* rm, const size_t frame_id);
  int32_t update(const size_t time) override;

  std::tuple<uint32_t, uint32_t> dims() const;
  std::string_view path() const;
private:
  render::resource_manager* rm;
  size_t frame_id;
  std::unique_ptr<ffmpeg_context> ctx;
  std::vector<ffmpeg_frame_data> queue;
  int64_t cur_timestamp;
};

}
}