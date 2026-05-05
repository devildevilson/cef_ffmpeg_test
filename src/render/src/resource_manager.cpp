#include "resource_manager.h"

#include "common/utils.h"
#include "system.h"

namespace cef_ffmpeg_test {
namespace render {

resource_manager::resource_manager() noexcept {}

size_t resource_manager::register_frame(system* sys, const uint32_t width, const uint32_t height, const enum common::format format, const enum common::buffering buffering) {
  const size_t id = resources.size();
  auto &r = resources.emplace_back();
  r.width = width;
  r.height = height;
  r.format = format;
  r.buffering = buffering;

  std::array<common::handle_t, 8> handles{};
  const auto check_size = sys->create(
    r.width, r.height, r.format, r.buffering, handles.data()
  );
  assert(check_size == static_cast<uint32_t>(r.buffering));

  for (uint32_t i = 0; i < static_cast<uint32_t>(r.buffering); ++i) {
    r.frames[i].handle = handles[i];
    r.frames[i].width = r.width;
    r.frames[i].height = r.height;
    r.frames[i].format = r.format;
  }

  return id;
}

void resource_manager::register_slot(const size_t id, const float x, const float y, const float w, const float h) {
  slots_arr.emplace_back(common::slot_t{id, x, y, w, h});
}

common::frame_t resource_manager::acquire_write_frame(const size_t id) const {
  // тут проверим есть ли свободный слот
  if (id >= resources.size()) common::error{UTILS_SOURCE_LOCATION_INIT}("Invalid resource slot {}, maximum {}", id, resources.size());
  const auto& res = resources[id];
  
  const size_t write_index = res.write.load(std::memory_order_relaxed);
  const size_t read_index = res.read.load(std::memory_order_acquire);
  const size_t buffer_count = static_cast<size_t>(res.buffering);

  if (write_index - read_index == buffer_count) {
    // свободных слотов нет, тут либо возвращаем невалидный фрейм
    // либо текущий, мы свободно можем возвращать текущий при тройной буферизации 
    // (и выше по идее, но зачем нам может потребоваться четверная буферизация?)
    if (res.buffering == common::buffering::triple) {
      const size_t index = write_index % buffer_count;
      return res.frames[index];
    }

    common::frame_t frame;
    memset(&frame, 0, sizeof(frame));
    return frame;
  }

  // возвращаем следующий свободный фрейм
  const size_t next_index = (write_index + 1) % buffer_count;
  return res.frames[next_index];
}

void resource_manager::swap_write_frame(const size_t id) {
  if (id >= resources.size()) common::error{UTILS_SOURCE_LOCATION_INIT}("Invalid resource slot {}, maximum {}", id, resources.size());
  auto& res = resources[id];

  // тут нужно увеличить read на 1
  const size_t write_index = res.write.load(std::memory_order_relaxed);
  const size_t read_index = res.read.load(std::memory_order_acquire);
  const size_t buffer_count = static_cast<size_t>(res.buffering);

  const bool has_slots = write_index - read_index < buffer_count;
  res.write.store(write_index + size_t(has_slots), std::memory_order_release);
}

common::frame_t resource_manager::acquire_read_frame(const size_t id) const {
  if (id >= resources.size()) common::error{UTILS_SOURCE_LOCATION_INIT}("Invalid resource slot {}, maximum {}", id, resources.size());
  auto& res = resources[id];

  const size_t read_index = res.read.load(std::memory_order_relaxed);
  // по идее должно помочь в синхронизации
  const size_t write_index = res.write.load(std::memory_order_acquire);
  const size_t buffer_count = static_cast<size_t>(res.buffering);
  (void)write_index;

  // тут неважно есть ли слоты или нет, всегда берем последний буфер
  //const bool has_slots = write_index - read_index > 0;
  // берем текущий буфер, он по идее уже должен быть заполнен еще с предыдущего кадра
  const size_t index = read_index % buffer_count;
  return res.frames[index];
}

void resource_manager::swap_read_frame(const size_t id) {
  if (id >= resources.size()) common::error{UTILS_SOURCE_LOCATION_INIT}("Invalid resource slot {}, maximum {}", id, resources.size());
  auto& res = resources[id];

  const size_t read_index = res.read.load(std::memory_order_relaxed);
  const size_t write_index = res.write.load(std::memory_order_acquire);
  const size_t buffer_count = static_cast<size_t>(res.buffering);

  // на этапе рендеринга, после того как кадр отрисовался 
  // мы можем вызвать этот метод чтобы получить актуальный буфер
  const bool has_slots = write_index - read_index > 0;
  res.read.store(read_index + size_t(has_slots), std::memory_order_release);
}

size_t resource_manager::frames_size() const { return resources.size(); }
const std::vector<common::slot_t>& resource_manager::slots() const { return slots_arr; }

resource_manager::frame_container_t::frame_container_t() noexcept :
  width(0), 
  height(0),
  format(common::format::RGBA),
  buffering(common::buffering::single),
  read(0),
  write(0),
  frames{}
{}

resource_manager::frame_container_t::frame_container_t(const frame_container_t& copy) noexcept :
  width(copy.width), 
  height(copy.height),
  format(copy.format),
  buffering(copy.buffering),
  read(copy.read.load(std::memory_order_acquire)),
  write(copy.write.load(std::memory_order_acquire)),
  frames(copy.frames)
{}

resource_manager::frame_container_t::frame_container_t(frame_container_t&& move) noexcept :
  width(move.width), 
  height(move.height),
  format(move.format),
  buffering(move.buffering),
  read(move.read.load(std::memory_order_acquire)),
  write(move.write.load(std::memory_order_acquire)),
  frames(move.frames)
{}

resource_manager::frame_container_t& resource_manager::frame_container_t::operator=(const frame_container_t&copy) noexcept {
  width = copy.width;
  height = copy.height;
  format = copy.format;
  buffering = copy.buffering;
  read = copy.read.load(std::memory_order_acquire);
  write = copy.write.load(std::memory_order_acquire);
  frames = copy.frames;

  return *this;
}

resource_manager::frame_container_t& resource_manager::frame_container_t::operator=(frame_container_t&& move) noexcept {
  width = move.width;
  height = move.height;
  format = move.format;
  buffering = move.buffering;
  read = move.read.load(std::memory_order_acquire);
  write = move.write.load(std::memory_order_acquire);
  frames = move.frames;

  return *this;
}

}
}