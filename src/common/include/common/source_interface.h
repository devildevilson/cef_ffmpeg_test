#pragma once

#include <cstddef>
#include <cstdint>

namespace cef_ffmpeg_test {
namespace common {

class source_interface {
public:
  virtual ~source_interface() noexcept = default;
  virtual int32_t update(const size_t time) = 0;
};

}
}