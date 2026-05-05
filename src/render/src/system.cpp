#include "system.h"

#include <vector>
#include <string>
#include <cassert>

#include "common/utils.h"
#include "resource_manager.h"

#ifdef _WIN32
#include <GL/glew.h>
#include <GL/wglew.h>
#else 
#include <GL/glew.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif

namespace cef_ffmpeg_test {
namespace render {

#define GL_CHECK_ERROR() \
  { \
    while (GLenum err = glGetError()) { \
      common::error{UTILS_SOURCE_LOCATION_INIT}("GL error {}", err); \
    } \
  } \


#define GL_FN_CHECK_ERROR(X) \
  X; \
  { \
    while (GLenum err = glGetError()) { \
      common::error{UTILS_SOURCE_LOCATION_INIT}("GL error {}", err); \
    } \
  } \

static GLuint compile_shader(GLenum type, const char* src) {
  auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  // проверка
  GLint ok;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    constexpr size_t buf_size = 512;
    char log[buf_size];
    glGetShaderInfoLog(shader, buf_size, nullptr, log);
    common::error{UTILS_SOURCE_LOCATION_INIT}("Shader error: {}", log);
  }

  return shader;
}

static GLuint create_program(const char* vs, const char* fs) {
  auto v = compile_shader(GL_VERTEX_SHADER, vs);
  auto f = compile_shader(GL_FRAGMENT_SHADER, fs);

  auto prog = glCreateProgram();
  glAttachShader(prog, v);
  glAttachShader(prog, f);
  glLinkProgram(prog);

  glDeleteShader(v);
  glDeleteShader(f);

  return prog;
}

enum class buffer_type {
  IMG, PBO
};

enum shader_type {
  DEFAULT_SHADER,
  CONVERSION_SHADER
};

// картинки + шейдеры
// как тут сделать адекватно?
struct system::render_context_t {
  struct texture_data {
    GLuint id;
    GLuint device_id;
    enum common::format format;
    uint32_t width, height;
    size_t size;
    uint8_t* ptr;

    inline texture_data() noexcept : 
      id(0), device_id(0), format(common::format::RGBA), width(0), height(0), size(0), ptr(nullptr)
    {}
  };

  struct device_texture {
    GLuint id;
  };

  struct shader_t {
    GLuint shader;
    GLint tex_location;
  };

  struct sync_t {
    GLuint tex;
    GLsync fence;

    inline sync_t() noexcept : tex(0), fence(nullptr) {}
  };

  render_context_t() : empty_vao(0) {
    glGenVertexArrays(1, &empty_vao);
    glBindVertexArray(empty_vao);
  }

  ~render_context_t() noexcept {
    glDeleteVertexArrays(1, &empty_vao);

    for (auto& s : shaders) {
      glDeleteProgram(s.shader);
    }

    for (auto& data : textures) {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data.id);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glDeleteBuffers(1, &data.id);
    }

    for (auto& tex : device_textures) {
      glDeleteTextures(1, &tex.id);
    }

    for (auto& fb : framebuffers) {
      if (fb.fence != nullptr) {
        glDeleteSync(fb.fence);
        fb.fence = nullptr;
      }
    }
  }

  GLuint empty_vao;
  std::vector<texture_data> textures;
  std::vector<device_texture> device_textures;
  std::vector<shader_t> shaders;
  std::vector<sync_t> framebuffers;
};

constexpr size_t framebuffer_count = 3;

system::system(const uint32_t width, const uint32_t height, resource_manager* rm) : width(width), height(height), rm(rm) {
  //native_render = std::make_unique<native_render_t>(width, height);
  //native_render->toggle_vsync();
  //this->width = native_render->width;
  //this->height = native_render->height;
  render_context = std::make_unique<render_context_t>();

  const auto vs_code = common::read_text("../resources/shaders/basic.vert");
  const auto fs_code = common::read_text("../resources/shaders/basic.frag");

  if (vs_code.empty()) {
    common::error{UTILS_SOURCE_LOCATION_INIT}("Could not load vertex shader '{}'", "../resources/shaders/basic.vert");
  }

  if (fs_code.empty()) {
    common::error{UTILS_SOURCE_LOCATION_INIT}("Could not load fragment shader '{}'", "../resources/shaders/basic.frag");
  }

  auto prog_id = create_program(vs_code.c_str(), fs_code.c_str());
  GLint location = glGetUniformLocation(prog_id, "tex");
  render_context->shaders.push_back({prog_id, location});

  render_context->framebuffers.resize(framebuffer_count);
}

system::~system() noexcept {}

size_t system::create(
    const uint32_t width, 
    const uint32_t height, 
    const enum common::format format,
    const enum common::buffering buffering,
    common::handle_t* handles
) {
  if (format == common::format::YUV420) {
    // создадим PBO + 3 картинки
    // точнее возможно потребуется на весь буферизированный PBO только 3 картинки
    // как сделать? нужно буферизацию сюда передать чтобы дать понять сколько чего мы хотим
  }

  // с другим форматом проще
  // может быть пока создавать RGBA? вообще имеет смысл

  auto& device_tex = render_context->device_textures.emplace_back();

  // так в OpenGL в принципе нельзя сделать сразу хост визибл текстурку 
  // придется копировать из буфера, а значит GPU картинка может быть одна
  // похоже что обязательно нужно добавить буферизацию в эту функцию
  device_tex.id = 0;
  glGenTextures(1, &device_tex.id);
  glBindTexture(GL_TEXTURE_2D, device_tex.id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  const GLenum glformat = format == common::format::BGRA ? GL_BGRA : GL_RGBA;

  // выделяем память
  glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    width,
    height,
    0,
    glformat,
    GL_UNSIGNED_BYTE,
    nullptr
  );

  glBindTexture(GL_TEXTURE_2D, 0); // =(

  // создали картинку куда будем копировать пиксели
  // теперь буферы этих пикселей

  for (size_t i = 0; i < static_cast<size_t>(buffering); ++i) {
    const size_t id = render_context->textures.size();
    auto& data = render_context->textures.emplace_back();

    data.device_id = device_tex.id;
    data.id = 0;
    glGenBuffers(1, &data.id);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data.id);

    data.size = size_t(width) * size_t(height) * sizeof(uint32_t);

    glBufferStorage(
      GL_PIXEL_UNPACK_BUFFER,
      data.size,
      nullptr,
      GL_MAP_WRITE_BIT |
      GL_MAP_PERSISTENT_BIT |
      GL_MAP_COHERENT_BIT
    );

    data.ptr = (uint8_t*)glMapBufferRange(
      GL_PIXEL_UNPACK_BUFFER,
      0,
      data.size,
      GL_MAP_WRITE_BIT |
      GL_MAP_PERSISTENT_BIT |
      GL_MAP_COHERENT_BIT
    );

    data.format = format;
    data.width = width;
    data.height = height;

    handles[i].id = id;
    handles[i].mem = data.ptr;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  return static_cast<size_t>(buffering);
}

int32_t system::update(const size_t time) {
  static const auto fn = [this] (const size_t index) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // для каждого уникального набора текстурок, обновим данные на устройстве
    for (size_t i = 0; i < rm->frames_size(); ++i) {
      const auto frame = rm->acquire_read_frame(i);
      const auto& data = render_context->textures[frame.handle.id];

      // биндим текстуру из PBO
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data.id);
      glBindTexture(GL_TEXTURE_2D, data.device_id);

      const GLenum glformat = frame.format == common::format::BGRA ? GL_BGRA : GL_RGBA;

      glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0, 0,
        data.width,
        data.height,
        glformat,
        GL_UNSIGNED_BYTE,
        (void*)0 // offset в PBO
      );

      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    // для каждого слота зададим свой вьюпорт и нарисуем там картинку
    const auto& slots = this->rm->slots();
    for (const auto& slot : slots) {
      const auto frame = rm->acquire_read_frame(slot.id);

      assert(frame.handle.id < render_context->textures.size());
      const auto& data = render_context->textures[frame.handle.id];
      
      // начало координат - левый верхний угол
      const uint32_t w = this->width  * slot.width;
      const uint32_t h = this->height * slot.height;
      const uint32_t x = this->width  * slot.x;
      const uint32_t y = this->height - (this->height * slot.y + h);
      glViewport(x, y, w, h);

      glDisable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);
      
      // укажем шейдер, укажем картинки, нарисуем
      const auto& program_data = this->render_context->shaders[0];
      glUseProgram(program_data.shader);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, data.device_id);
      glUniform1i(program_data.tex_location, 0);

      glBindVertexArray(this->render_context->empty_vao);

      glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
    }

    this->render_context->framebuffers[index].fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    GL_CHECK_ERROR()
  };

  const size_t current_frame_index = counter.fetch_add(1, std::memory_order_relaxed);
  const size_t normalized_index = current_frame_index % framebuffer_count;

  // подождем фенс и продвинем read указатель
  auto& f = render_context->framebuffers[normalized_index].fence;
  if (f != nullptr) {
    glClientWaitSync(f, GL_SYNC_FLUSH_COMMANDS_BIT, UINT64_MAX);
    glDeleteSync(f);
    f = nullptr;
  }

  for (size_t i = 0; i < rm->frames_size(); ++i) {
    rm->swap_read_frame(i);
  }

  //return native_render->update(time, fn, normalized_index);
  std::invoke(fn, normalized_index);
  return 0;
}

}
}