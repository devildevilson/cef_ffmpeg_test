#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "common/source_interface.h"

namespace cef_ffmpeg_test {
namespace render {
class resource_manager;
class system;
}

namespace source {

struct cef_context;

class cef : public common::source_interface {
public:
  cef(std::string url, const uint32_t width, const uint32_t height, const float scale = 1.0f);
  ~cef() noexcept;

  void set_update_manager(render::resource_manager* rm, const size_t frame_id);
  int32_t update(const size_t time) override; // ничего не делаем

  std::tuple<uint32_t, uint32_t> dims() const;

  void close_browser();
private:
  std::unique_ptr<cef_context> ctx;
};

// может вернуть код 0 или больше, если так то нужно выйти из main
int cef_main(int argc, char* argv[]);
// сделаем часть работы, должно вызываться в том же потоке что и cef_main
void cef_do_work();
void cef_shutdown();
void cef_shutdown_fast();

}
}