#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

#include <spdlog/spdlog.h>

namespace cef_ffmpeg_test {
namespace common {

#ifdef _WIN32 
#  define UTILS_FUNCTION_SIGNATURE __FUNCSIG__
#else
#  define UTILS_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif
#define UTILS_SOURCE_LOCATION_INIT cef_ffmpeg_test::common::source_loc(__LINE__, 0, __FILE__, UTILS_FUNCTION_SIGNATURE)

struct source_loc {
  uint32_t line, column;
  std::string_view file_name;
  std::string_view function_name;

  constexpr inline source_loc(
    const uint32_t line, 
    const uint32_t column,
    const std::string_view &file_name,
    const std::string_view &function_name
  ) noexcept :
    line(line), column(column), file_name(file_name), function_name(function_name)
  {}
};

struct error {
  source_loc loc;
  explicit constexpr error(source_loc l) noexcept : loc(l) {}
  template <typename... Args>
  constexpr void operator()(const spdlog::format_string_t<Args...>& format, Args&&... args) const {
    spdlog::error(format, std::forward<Args>(args)...);
    throw std::runtime_error(fmt::format("{}:{}: {}: Got runtime error", loc.file_name, loc.line, loc.function_name));
  }
};

//template <typename... Args> 
//void error(const std::format_string<Args...> &format, Args&&... args, std::source_location loc = std::source_location::current()) {
//  spdlog::error(format, std::forward<Args>(args)...);
//  throw std::runtime_error(std::format("{}:{}: {}: Got runtime error", make_sane_file_name(loc.file_name()), loc.line(), loc.function_name()));
//}

template <typename... Args>
constexpr void info(const spdlog::format_string_t<Args...> &format, Args&&... args) {
  spdlog::info(format, std::forward<Args>(args)...);
}

template <typename... Args>
constexpr void warn(const spdlog::format_string_t<Args...> &format, Args&&... args) {
  spdlog::warn(format, std::forward<Args>(args)...);
}

enum class file_type { text, binary };
std::string read_text(std::string path, const enum file_type type = file_type::text);
std::vector<char> read_bin(std::string path, const enum file_type type = file_type::binary);
std::string make_full_path(const std::string& path);

bool app_should_stop();
}
}