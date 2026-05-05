#include <chrono>

#include <cassert>

#include "render/system.h"
#include "render/resource_manager.h"
#include "render/window.h"
#include "common/utils.h"
#include "common/settings.h"
#include "source/ffmpeg.h"
#include "source/cef.h"

using namespace cef_ffmpeg_test;

class application {
public:
  application(const common::settings& sets, const uint32_t surface_w, const uint32_t surface_h) {
    rm = std::make_unique<render::resource_manager>();
    s = std::make_unique<render::system>(surface_w, surface_h, rm.get());

    for (const auto& source : sets.sources) {
      if (source.type == "ffmpeg") {
        auto ptr = new source::ffmpeg(source.resource);
        const auto [w, h] = ptr->dims();

        const size_t id = rm->register_frame(s.get(), w, h, common::format::RGBA, common::buffering::edouble);
        ptr->set_update_manager(rm.get(), id);

        for (const auto& slot : sets.slots) {
          if (slot.name == source.name) {
            rm->register_slot(id, slot.x, slot.y, slot.width, slot.height);
          }
        }

        std::unique_ptr<common::source_interface> i(ptr);
        sources.emplace_back(std::move(i));
        continue;
      }

      if (source.type == "cef") {
        const auto str = common::make_full_path(source.resource);
        auto ptr = new source::cef("file://" + str, source.width, source.height, source.scale);
        const auto [w, h] = ptr->dims();

        const size_t id = rm->register_frame(s.get(), w, h, common::format::BGRA, common::buffering::edouble);
        ptr->set_update_manager(rm.get(), id);

        for (const auto& slot : sets.slots) {
          if (slot.name == source.name) {
            rm->register_slot(id, slot.x, slot.y, slot.width, slot.height);
          }
        }

        std::unique_ptr<common::source_interface> i(ptr);
        sources.emplace_back(std::move(i));
        continue;
      }
    }
  }

  ~application() {
    
  }

  int32_t run(const size_t time) {
    for (auto& ptr : sources) {
      const int32_t ret = ptr->update(time);
      assert(ret == 0);
    }

    const int32_t ret = s->update(time);
    assert(ret == 0);

    return 0;
  }
private:
  std::unique_ptr<render::resource_manager> rm;
  std::unique_ptr<render::system> s;
  std::vector<std::unique_ptr<common::source_interface>> sources;
};

int main(int argc, char* argv[]) {
  common::info("Hello");

  {
    const int ret = source::cef_main(argc, argv); 
    // сабпроцессы
    if (ret >= 0) {
      source::cef_shutdown_fast();
      return ret; 
    }

    common::info("Start");
    const auto settings = common::parse_settings();
    render::window wnd(settings.window.width, settings.window.height);

    const auto [sw, sh] = wnd.dims();
    application app(settings, sw, sh);

    size_t counter = 0; 
    auto tp = std::chrono::steady_clock::now();
    while (!wnd.should_close()) {
      counter += 1;
      wnd.update();

      source::cef_do_work();
      app.run(16667);
      wnd.swap_buffers();

      std::this_thread::sleep_until(tp + counter * std::chrono::microseconds(16667));
    }

    common::info("Start shutdown");
  }

  source::cef_shutdown_fast();

  return 0;
}