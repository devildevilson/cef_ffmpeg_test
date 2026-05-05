#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>
#include <vector>

#include "common/common.h"

namespace cef_ffmpeg_test {
namespace render {

class resource_manager;

/**
* система заберет из ресурс менеджера текущий список картинок с которыми сейчас работаем
* и отправит их в разные шейдеры в зависимости от формата
* тут еще будет "производство" хендлов для картинок
* тут же наверное будет еще окно
*/
class system {
public:
  system(const uint32_t width, const uint32_t height, resource_manager* rm);
  ~system() noexcept;

  size_t create(
    const uint32_t width, 
    const uint32_t height, 
    const enum common::format format,
    const enum common::buffering buffering,
    common::handle_t* handles
  );

  int32_t update(const size_t time);
private:
  // окно
  struct native_render_t;
  // хранилище картинок + шейдеры
  struct render_context_t;

  //std::unique_ptr<native_render_t> native_render;
  std::unique_ptr<render_context_t> render_context;

  uint32_t width, height;

  resource_manager* rm;

  std::atomic<size_t> counter;

};

}
}