#include "utils.h"

#include <fstream>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace cef_ffmpeg_test {
namespace common {

std::atomic<size_t> g_exit_not_ready(0);
std::atomic<bool> g_closing(false);

std::string read_text(std::string path, const enum file_type type) {
  const auto mode = type == file_type::binary ? std::ios::in | std::ios::binary : std::ios::in;
  std::fstream file(std::move(path), mode);
  if (!file.is_open()) return std::string();
  return std::string(
    std::istreambuf_iterator<char>(file),
    std::istreambuf_iterator<char>()
  );
}

std::vector<char> read_bin(std::string path, const enum file_type type) {
  const auto mode = type == file_type::binary ? std::ios::in | std::ios::binary : std::ios::in;
  std::fstream file(std::move(path), mode);
  if (!file.is_open()) return std::vector<char>();
  return std::vector<char>(
    std::istreambuf_iterator<char>(file),
    std::istreambuf_iterator<char>()
  );
}

std::string make_full_path(const std::string& path) {
  return fs::absolute(path).generic_string();
}

//bool app_should_stop() {
//  bool stop = false;
//  MSG msg;
//  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
//    if (msg.message == WM_CLOSE) stop = true;
//    if (msg.message == WM_QUIT) stop = true;
//
//    TranslateMessage(&msg);
//    DispatchMessage(&msg);
//  }
//
//  return stop;
//}

}
}