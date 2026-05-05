#include "window.h"

#include <vector>
#include <string>
#include <cassert>

#include "common/utils.h"
#include "common/common.h"
#include "resource_manager.h"

#ifdef _WIN32
#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#else
#include <X11/Xlib.h>
#include <GL/glew.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#define APIENTRY
#endif

namespace cef_ffmpeg_test {
namespace render {

void APIENTRY glDebugCallback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam
);

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_CLOSE) {
    common::info("Got msg WM_CLOSE, g_closing = {}, g_exit_not_ready = {}", common::g_closing.load(), common::g_exit_not_ready.load());
    common::g_closing = true;
    if (common::g_exit_not_ready.load() != 0) {
      // нужно закрыть все браузеры
      return 0;
    }

    // идем дальше
  }

  if (msg == WM_DESTROY) {
    common::info("Got msg WM_DESTROY, g_closing = {}, g_exit_not_ready = {}", common::g_closing.load(), common::g_exit_not_ready.load());
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

struct native_window_t {
  HWND hwnd;
  HDC hdc;
  HGLRC ctx;
  MSG msg;
  bool vsync;

  uint32_t width, height;

  native_window_t(const uint32_t width, const uint32_t height) : hwnd(nullptr), hdc(nullptr), ctx(nullptr), msg{}, vsync(false), width(0), height(0) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "GLWindow";

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
      0,
      wc.lpszClassName,
      "OpenGL Window",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      width, height,
      nullptr, nullptr,
      wc.hInstance,
      nullptr
    );

    ShowWindow(hwnd, SW_SHOW);

    hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC dummy = wglCreateContext(hdc);
    wglMakeCurrent(hdc, dummy);

    glewExperimental = GL_TRUE;
    glewInit();

    const auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");

    int attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 5,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0
    };

    ctx = wglCreateContextAttribsARB(hdc, 0, attribs);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy);
    wglMakeCurrent(hdc, ctx);

    glewExperimental = GL_TRUE;
    glewInit();

    common::info("{}", std::string_view((const char*)glGetString(GL_VERSION)));

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glDebugMessageCallback(glDebugCallback, nullptr);

    RECT r;
    GetClientRect(hwnd, &r);
    common::info("Client window size = {} x {}", r.right, r.bottom);

    this->width = r.right;
    this->height = r.bottom;
  }

  ~native_window_t() noexcept {
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(ctx);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    // нужно?
    //UnregisterClass(wc.lpszClassName, wc.hInstance);
  }

  void toggle_vsync() {
    vsync = !vsync;
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    wglSwapIntervalEXT(vsync);
  }

  // добавим коллбек? имеет смысл
  template <typename F, typename... Args>
  int32_t update(const size_t time, F f, Args&&... args) {
    /*while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) { 
        common::info("msg.message == WM_QUIT");
        return 1;
      }

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }*/

    // render
    std::invoke(f, std::forward<Args>(args)...);

    SwapBuffers(hdc);

    return 0;
  }

  void swap_buffers() const {
    SwapBuffers(hdc);
  }
};

#else

struct native_window_t {
  Display* x_display;
  Window win;
  EGLDisplay egl_display;
  EGLSurface surface;
  EGLContext context;

  Atom WM_DELETE_WINDOW;

  uint32_t width, height;

  native_window_t(const uint32_t width, const uint32_t height) : width(0), height(0) {
    x_display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(x_display);

    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask;

    win = XCreateWindow(
      x_display, root,
      0, 0, width, height,
      0,
      CopyFromParent,
      InputOutput,
      CopyFromParent,
      CWEventMask,
      &swa
    );

    XMapWindow(x_display, win);
    XStoreName(x_display, win, "EGL + OpenGL 4.5");

    egl_display = eglGetDisplay((EGLNativeDisplayType)x_display);
    eglInitialize(egl_display, nullptr, nullptr);

    EGLint config_attribs[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_NONE
    };

    EGLConfig config;
    EGLint num_configs;
    eglChooseConfig(egl_display, config_attribs, &config, 1, &num_configs);

    surface = eglCreateWindowSurface(
      egl_display,
      config,
      (EGLNativeWindowType)win,
      nullptr
    );

    eglBindAPI(EGL_OPENGL_API);

    EGLint context_attribs[] = {
      EGL_CONTEXT_MAJOR_VERSION, 4,
      EGL_CONTEXT_MINOR_VERSION, 5,
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_NONE
    };

    context = eglCreateContext(
      egl_display,
      config,
      EGL_NO_CONTEXT,
      context_attribs
    );

    eglMakeCurrent(egl_display, surface, surface, context);

    common::info("{}", std::string_view((const char*)glGetString(GL_VERSION)));

    glewExperimental = GL_TRUE;   // важно для core profile
    GLenum err = glewInit();

    // ???
    if (err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("GLEW error: {}", std::string_view((const char*)glewGetErrorString(err)));
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugCallback, nullptr);

    //XWindowAttributes attrs;
    //XGetWindowAttributes(x_display, win, &attrs);
    //this->width  = attrs.width;
    //this->height = attrs.height;

    EGLint w, h;
    eglQuerySurface(egl_display, surface, EGL_WIDTH,  &w);
    eglQuerySurface(egl_display, surface, EGL_HEIGHT, &h);
    this->width  = w;
    this->height = h;

    common::info("Client window size = {} x {}", this->width, this->height);

    WM_DELETE_WINDOW = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(x_display, win, &WM_DELETE_WINDOW, 1);
  }

  ~native_window_t() {
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(egl_display, context);
    eglDestroySurface(egl_display, surface);
    eglTerminate(egl_display);
    eglReleaseThread();

    XDestroyWindow(x_display, win);
    XCloseDisplay(x_display);
  }

  void swap_buffers() const {
    eglSwapBuffers(egl_display, surface);
  }
};

#endif

void APIENTRY glDebugCallback(
  GLenum,
  GLenum,
  GLuint,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void*
) {
  if (severity == GL_DEBUG_SEVERITY_HIGH) {
    common::error{UTILS_SOURCE_LOCATION_INIT}("GL DEBUG: {}", std::string_view(message, length));
  }

  if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
    common::warn("GL DEBUG: {}", std::string_view(message, length));
  }
}

window::window(const uint32_t width, const uint32_t height) {
  wnd = std::make_unique<native_window_t>(width, height);
}

window::~window() noexcept {}

void window::update() {
#ifndef _WIN32
  while (XPending(wnd->x_display)) {
    XEvent event;
    XNextEvent(wnd->x_display, &event);

    switch (event.type) {
      case Expose: break;

      case ClientMessage:
        // готовимся закрывать окно
        if ((Atom)event.xclient.data.l[0] == wnd->WM_DELETE_WINDOW) {
          common::g_closing = true;
        }
        break;
    }
  }
#endif
}

bool window::should_close() {
  return common::g_exit_not_ready.load() == 0;
}

void window::swap_buffers() {
  wnd->swap_buffers();
}

std::tuple<uint32_t, uint32_t> window::dims() const {
  return std::make_tuple(wnd->width, wnd->height);
}

}
}