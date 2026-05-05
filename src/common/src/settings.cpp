#include "settings.h"

#include "utils.h"
#include "cpp-json/json.h"

namespace cef_ffmpeg_test {
namespace common {

const auto default_settings = settings{
  {
    settings::source_t{
      "video1",
      "ffmpeg",
      "../resources/videos/test1.mp4",
      0,0, 1.0f
    },

    settings::source_t{
      "video2",
      "ffmpeg",
      "../resources/videos/test2.mp4",
      0,0, 1.0f
    },

    settings::source_t{
      "page1",
      "cef",
      "../resources/htmls/index1.html",
      800, 600, 1.0f
    },
  },
  {
    settings::slot_t{ "page1", 0.0f, 0.0f, 1.0f, 0.5f },
    settings::slot_t{ "video1", 0.0f, 0.5f, 0.5f, 0.5f },
    settings::slot_t{ "video2", 0.5f, 0.5f, 0.5f, 0.5f },
  },
  settings::window_t{ 1280, 720 }
};

settings parse_settings(std::string path) {
  const auto text = common::read_text(std::move(path));
  if (text.empty()) {
    common::warn("Could not load 'settings.json'. Using default settings");
    return default_settings;
  }

  settings s = default_settings;

  json::value json = json::parse(text);
  if (json["window"].is_array()) {
    s.window.width = json["window"][0].is_number() ? json::to_number<uint32_t>(json["window"][0]) : s.window.width;
    s.window.height = json["window"][0].is_number() ? json::to_number<uint32_t>(json["window"][1]) : s.window.height;
  }

  if (json["sources"].is_array()) {
    s.sources.clear();
    const auto arr = json["sources"].as_array();
    for (const auto& obj : arr) {
      if (!obj.is_object()) continue;

      settings::source_t ss{};
      if (obj["name"].is_string()) {
        ss.name = obj["name"].as_string();
      }

      if (obj["type"].is_string()) {
        ss.type = obj["type"].as_string();
      }

      if (obj["resource"].is_string()) {
        ss.resource = obj["resource"].as_string();
      }

      if (obj["width"].is_number()) {
        ss.width = json::to_number<uint32_t>(obj["width"]);
      }

      if (obj["height"].is_number()) {
        ss.height = json::to_number<uint32_t>(obj["height"]);
      }

      if (obj["scale"].is_number()) {
        ss.scale = json::to_number<float>(obj["scale"]);
      }

      s.sources.emplace_back(std::move(ss));
    }
  }

  if (json["slots"].is_array()) {
    s.slots.clear();
    const auto arr = json["slots"].as_array();
    for (const auto& obj : arr) {
      if (!obj.is_array()) continue;

      settings::slot_t ss{};
      if (obj[0].is_string()) {
        ss.name = obj[0].as_string();
      }

      if (obj[1].is_number()) {
        ss.x = json::to_number<float>(obj[1]);
      }

      if (obj[2].is_number()) {
        ss.y = json::to_number<float>(obj[2]);
      }

      if (obj[3].is_number()) {
        ss.width = json::to_number<float>(obj[3]);
      }

      if (obj[4].is_number()) {
        ss.height = json::to_number<float>(obj[4]);
      }

      s.slots.emplace_back(std::move(ss));
    }
  }

  return s;
}

}
}