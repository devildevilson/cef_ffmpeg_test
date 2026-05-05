#include "ffmpeg.h"

#include "common/utils.h"
#include "render/resource_manager.h"
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace cef_ffmpeg_test {
namespace source {

constexpr int64_t one_second = 1000000;

enum ffmpeg_result {
  FFMPEG_SUCCESS,
  FFMPEG_EOF,
  FFMPEG_ERROR,
  count
};

struct ffmpeg_context {
  std::string path;
  AVFormatContext* fmt;
  AVCodecContext* codec_ctx;
  SwsContext* sws;
  AVFrame* frame; // YUV
  AVFrame* rgba_frame;
  uint8_t* buffer;
  AVPacket pkt;

  uint32_t video_stream_index, width, height;

  ffmpeg_context(std::string p) : 
    path(std::move(p)),
    fmt(nullptr), 
    codec_ctx(nullptr),
    sws(nullptr),
    frame(nullptr),
    rgba_frame(nullptr),
    buffer(nullptr),
    pkt{},
    video_stream_index(UINT32_MAX), width(0), height(0)
  {
    // option?
    if (avformat_open_input(&fmt, path.c_str(), nullptr, nullptr) < 0) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not open input '{}'", path);
    }

    if (avformat_find_stream_info(fmt, nullptr) < 0) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not find stream info from '{}'", path);
    }

    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
      if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        video_stream_index = i;
        break;
      }
    }

    if (video_stream_index == UINT32_MAX) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not find video stream from '{}'", path);
    }

    AVCodecParameters* codecpar = fmt->streams[video_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);

    codec_ctx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not fill codec params from '{}'", path);
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not allocate codec context from '{}'", path);
    }

    this->width = codec_ctx->width;
    this->height = codec_ctx->height;

    sws = sws_getContext(
      codec_ctx->width,
      codec_ctx->height,
      codec_ctx->pix_fmt,

      codec_ctx->width,
      codec_ctx->height,
      AV_PIX_FMT_RGBA,

      SWS_BILINEAR,
      nullptr, nullptr, nullptr
    );

    if (sws == nullptr) {
      common::error{UTILS_SOURCE_LOCATION_INIT}("Could not create converter for '{}'", path);
    }

    frame = av_frame_alloc();
    if (frame == nullptr) common::error{UTILS_SOURCE_LOCATION_INIT}("Could not allocate frame");
    rgba_frame = av_frame_alloc();
    if (rgba_frame == nullptr) common::error{UTILS_SOURCE_LOCATION_INIT}("Could not allocate frame");

    auto size = av_image_get_buffer_size(
      AV_PIX_FMT_RGBA,
      codec_ctx->width,
      codec_ctx->height,
      1
    );

    uint8_t* buffer = (uint8_t*)av_malloc(size);

    auto req_size = av_image_fill_arrays(
      rgba_frame->data,
      rgba_frame->linesize,
      buffer,
      AV_PIX_FMT_RGBA,
      codec_ctx->width,
      codec_ctx->height,
      1
    );

    if (req_size < 0) common::error{UTILS_SOURCE_LOCATION_INIT}("Could not prepare ffmpeg decoder");


  }

  ~ffmpeg_context() {
    av_free(buffer);
    av_frame_free(&frame);
    av_frame_free(&rgba_frame);

    sws_freeContext(sws);

    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt);
  }

  double get_time_base() const {
    auto stream = fmt->streams[video_stream_index];
    return av_q2d(stream->time_base);
  }

  int32_t decode_next_frame(ffmpeg_frame_data& fr) {
    bool has_frame = false;
    while (!has_frame) {
      if (av_read_frame(fmt, &pkt) < 0) return FFMPEG_EOF;

      if (pkt.stream_index != video_stream_index) {
        av_packet_unref(&pkt);
        continue;
      }

      avcodec_send_packet(codec_ctx, &pkt);
      av_packet_unref(&pkt); // отправили в кодек, освободим память
      auto ret = avcodec_receive_frame(codec_ctx, frame);

      // нужно больше пакетов
      if (ret == AVERROR(EAGAIN)) continue;
      if (ret < 0) return FFMPEG_ERROR;

      // перепакуем в RGBA
      sws_scale(
        sws,
        frame->data,
        frame->linesize,
        0,
        codec_ctx->height,
        rgba_frame->data,
        rgba_frame->linesize
      );

      fr.pixels = rgba_frame->data[0];
      fr.stride = rgba_frame->linesize[0];
      fr.timestamp = (double(frame->best_effort_timestamp) * get_time_base()) * one_second;
      has_frame = true;
    }

    return FFMPEG_SUCCESS;
  }

  void seek_start() {
    av_seek_frame(fmt, video_stream_index, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codec_ctx);
  }
};

ffmpeg::ffmpeg(std::string path, const size_t queue_size) : rm(nullptr), frame_id(SIZE_MAX), cur_timestamp(INT64_MAX) {
  queue.reserve(queue_size);

  ctx = std::make_unique<ffmpeg_context>(std::move(path));
}

ffmpeg::~ffmpeg() noexcept {}

void ffmpeg::set_update_manager(render::resource_manager* rm, const size_t frame_id) {
  this->rm = rm;
  this->frame_id = frame_id;
}

int32_t ffmpeg::update(const size_t time) {
  while (queue.size() < queue.capacity()) {
    ffmpeg_frame_data fr{};
    const auto ret = ctx->decode_next_frame(fr);
    if (ret == FFMPEG_ERROR) common::error{UTILS_SOURCE_LOCATION_INIT}("Could not decode frame");
    // если видос меньше 3 фреймов - все погибнет 
    if (ret == FFMPEG_EOF) { 
      ctx->seek_start(); 
      cur_timestamp = INT64_MAX;
      continue; 
    }

    assert(ret == FFMPEG_SUCCESS);
    auto itr = std::upper_bound(queue.begin(), queue.end(), fr.timestamp, [] (const auto &a, const auto &b) {
      return std::less<int64_t>{}(a, b.timestamp);
    });

    queue.insert(itr, fr);
  }
  
  if (queue.empty()) return 1; // закончили?

  if (cur_timestamp == INT64_MAX) {
    cur_timestamp = queue.front().timestamp;
  }
  
  cur_timestamp += time;
  if (cur_timestamp >= queue.front().timestamp) {
    // отправляем пакет в менеджер ресурсов

    const auto fr = queue.front();
    queue.erase(queue.begin()); // быстрый memmove на pod типы

    if (rm == nullptr) return 1; // вообще ошибка
    if (frame_id == SIZE_MAX) return 1; // вообще ошибка
    auto render_frame = rm->acquire_write_frame(frame_id);
    // такое возможно, мы не перезаписываем фреймы если они не успели нарисоваться
    // точнее на тройной буферизации свободно перезаписываем
    // на остальных дропаем, наверное при больших лагах декодера
    // начнутся проблемы с data race...
    if (!render_frame.valid()) return 0;

    auto dst = reinterpret_cast<uint8_t*>(render_frame.handle.mem);

    const auto [width, height] = dims();
    assert(render_frame.width == width);
    assert(render_frame.height == height);
    for (uint32_t y = 0; y < height; ++y) {
      memcpy(
        dst + y * width * sizeof(uint32_t),
        fr.pixels + y * fr.stride,
        width * sizeof(uint32_t)
      );
    }

    rm->swap_write_frame(frame_id);
  }

  return 0;
}

std::tuple<uint32_t, uint32_t> ffmpeg::dims() const {
  return std::make_tuple(ctx->width, ctx->height);
}

std::string_view ffmpeg::path() const {
  return std::string_view(ctx->path);
}

}
}