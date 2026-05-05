#include "cef.h"

// боже мой винда =(
#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include "include/cef_browser.h"
#include "include/cef_browser_process_handler.h"

#include "render/resource_manager.h"
#include "common/utils.h"
#include "common/common.h"

namespace cef_ffmpeg_test {
namespace source {

class cef_client : public CefClient, public CefRenderHandler, public CefLifeSpanHandler {
public:
  uint32_t width, height;
  float scale;
  render::resource_manager* rm;
  size_t id;
  CefRefPtr<CefBrowser> browser;
  bool closing_flag;

  cef_client(const uint32_t width, const uint32_t height, const float scale) noexcept : 
    width(width), 
    height(height),
    scale(scale),
    rm(nullptr),
    id(SIZE_MAX),
    closing_flag(false)
  {
    common::g_exit_not_ready.fetch_add(1);
  }

  ~cef_client() noexcept {
    
  }

  CefRefPtr<CefRenderHandler> GetRenderHandler() override {
    return this;
  }

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
    return this;
  }

  void GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) override {
    rect = CefRect(0, 0, width * scale, height * scale);
  }

  // указываем размер окна в котором рендерится браузер
  bool GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo& screen_info) override {
    screen_info.device_scale_factor = scale;
    screen_info.rect = CefRect(0, 0, width * scale, height * scale);
    screen_info.available_rect = screen_info.rect;
    return true;
  }

  void OnAfterCreated(CefRefPtr<CefBrowser> b) override {
    common::info("OnAfterCreated");
    browser = b;
  }

  void OnPaint(
    CefRefPtr<CefBrowser>,
    PaintElementType,
    const RectList&,
    const void* buffer,
    int width,
    int height
  ) override {
    // отдает пиксели в формате BGRA + метод вызывается в потоке браузера
    // но нам похоже что все равно, сюда просто нужно передать менеджер ресурсов

    // у opengl есть GL_BGRA формат

    if (rm == nullptr) return;
    if (id == SIZE_MAX) return;

    const auto f = rm->acquire_write_frame(id);
    if (!f.valid()) return; // дропаем кадр

    assert(width == f.width);
    assert(height == f.height);

    // по идее CEF гарантирует что данные всегда 32бита + упакованы рядом друг с другом
    memcpy(f.handle.mem, buffer, size_t(width) * size_t(height) * sizeof(uint32_t));
    rm->swap_write_frame(id);
  }

  // логика закрытия в CEF отвратительная
  // задушили меня насмерть =(
  bool DoClose(CefRefPtr<CefBrowser> browser) {
    closing_flag = true;
    common::info("DoClose");

    common::g_exit_not_ready.fetch_add(-1);
#ifdef _WIN32
    // отправим еще одно сообщение окну что хотим закрыть его
    HWND hWnd = browser->GetHost()->GetWindowHandle();
    PostMessage(hWnd, WM_CLOSE, 0, 0);
#endif

    return false;
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    this->browser = nullptr;
    //g_ApplicationRunning = false; 
    common::info("OnBeforeClose");
  }

  void close_browser() {
    if (closing_flag) return;
    browser->GetHost()->CloseBrowser(true);
  }

  IMPLEMENT_REFCOUNTING(cef_client); 
};

struct cef_context {
  std::string url;
  CefRefPtr<cef_client> client;

  cef_context(std::string _url, const uint32_t width, const uint32_t height, const float scale) : url(std::move(_url)) {
    CefWindowInfo window_info;
#ifdef _WIN32
    window_info.SetAsWindowless(nullptr); // OSR
#else
    window_info.SetAsWindowless(0); // OSR
#endif

    CefBrowserSettings browser_settings;
    
    client = new cef_client(width, height, scale);

    CefBrowserHost::CreateBrowser(
      window_info,
      client,
      url, 
      browser_settings,
      nullptr,
      nullptr
    );
  }

  ~cef_context() noexcept {
    //client->close_browser();
  }

  int32_t update(const size_t time) {
    //CefDoMessageLoopWork();
    if (common::g_closing.load()) {
      client->close_browser();
    }

    return 0;
  }
};

cef::cef(std::string url, const uint32_t width, const uint32_t height, const float scale) {
  ctx = std::make_unique<cef_context>(std::move(url), width, height, scale);
}

cef::~cef() noexcept {
  
}

// должно вызываться в main потоке
void cef::set_update_manager(render::resource_manager* rm, const size_t frame_id) {
  this->ctx->client->rm = rm;
  this->ctx->client->id = frame_id;
}

int32_t cef::update(const size_t time) {
  return ctx->update(time);
}

std::tuple<uint32_t, uint32_t> cef::dims() const {
  const auto w = this->ctx->client->width;
  const auto h = this->ctx->client->height;
  const auto s = this->ctx->client->scale;
  return std::make_tuple(w * s, h * s);
}

void cef::close_browser() {
  ctx->client->close_browser();
}

int cef_main(int argc, char* argv[]) {
#ifdef WIN32
  CefMainArgs main_args(GetModuleHandle(nullptr));
  (void)argc; (void)argv;
#else
  CefMainArgs main_args(argc, argv);
#endif

  // subprocess handling
  int exit_code = CefExecuteProcess(main_args, nullptr, nullptr);
  if (exit_code >= 0) return exit_code;

  CefSettings settings;
  settings.no_sandbox = true;
  settings.windowless_rendering_enabled = true; // OSR
  //settings.multi_threaded_message_loop = false; 

  CefInitialize(main_args, settings, nullptr, nullptr);
  return -1;
}

void cef_do_work() {
  CefDoMessageLoopWork();
}

void cef_shutdown() {
  CefRunMessageLoop(); // нужно чтобы аккуратно завершить CEF
  CefShutdown();
}

void cef_shutdown_fast() {
  CefShutdown();
}
}
}