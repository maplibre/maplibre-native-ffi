#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

// The DRM fourcc codes that a session renders, and the linear modifier, from
// drm_fourcc.h.
inline constexpr uint32_t drm_format_abgr8888 = 0x34324241;
inline constexpr uint32_t drm_format_argb8888 = 0x34325241;
inline constexpr uint64_t drm_format_mod_linear = 0;

// The ring that a dmabuf attach asks for.
struct DmaBufRing {
  uint32_t buffer_count = 0;
  // The host's formats, most preferred first.
  std::vector<mln_dma_buf_format> formats;
};

// The fourcc that a session renders, with its modifiers that both the host
// and the graphics context take, most preferred first.
struct DmaBufChoice {
  uint32_t fourcc = 0;
  std::vector<uint64_t> modifiers;
};

// Picks the first of the host's fourccs that a session renders and for which
// `supported` takes a modifier. Returns false when none has one.
template <typename Supported>
auto choose_dma_buf_format(
  const std::vector<mln_dma_buf_format>& formats, Supported supported,
  DmaBufChoice& out_choice
) -> bool {
  for (const auto& candidate : formats) {
    if (
      candidate.fourcc != drm_format_abgr8888 &&
      candidate.fourcc != drm_format_argb8888
    ) {
      continue;
    }
    auto choice = DmaBufChoice{.fourcc = candidate.fourcc, .modifiers = {}};
    for (const auto& format : formats) {
      if (format.fourcc == choice.fourcc && supported(format)) {
        choice.modifiers.push_back(format.modifier);
      }
    }
    if (!choice.modifiers.empty()) {
      out_choice = std::move(choice);
      return true;
    }
  }
  return false;
}

// A file descriptor that a ring owns and lends to acquired frames.
class OwnedFd {
 public:
  OwnedFd() = default;
  explicit OwnedFd(int fd) : fd_(fd) {}
  OwnedFd(const OwnedFd&) = delete;
  auto operator=(const OwnedFd&) -> OwnedFd& = delete;
  OwnedFd(OwnedFd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
  auto operator=(OwnedFd&& other) noexcept -> OwnedFd& {
    if (this != &other) {
      close_fd(fd_);
      fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
  }
  ~OwnedFd() { close_fd(fd_); }

  [[nodiscard]] auto get() const -> int { return fd_; }

 private:
  int fd_ = -1;
};

// Where a ring's dmabuf keeps its pixels. The ring owns `fd`.
struct DmaBufLayout {
  uint32_t fourcc = 0;
  uint64_t modifier = 0;
  int fd = -1;
  uint32_t offset = 0;
  uint32_t stride = 0;
};

// What an acquired frame of a dmabuf ring reports.
inline auto dma_buf_frame(
  const DmaBufLayout& layout, const RenderFrameMetadata& frame
) -> mln_dma_buf_frame {
  return mln_dma_buf_frame{
    .size = sizeof(mln_dma_buf_frame),
    .width = frame.physical_width,
    .height = frame.physical_height,
    .scale_factor = frame.scale_factor,
    .slot = frame.slot,
    .fourcc = layout.fourcc,
    .modifier = layout.modifier,
    .fd = layout.fd,
    .offset = layout.offset,
    .stride = layout.stride,
  };
}

// Prepares a new session to render into `ring`, on the attaching thread. Each
// build's render backend defines this. The Vulkan build renders with `device`,
// or a device of the library's when it is null. The EGL build takes only a
// null `device`.
auto configure_dma_buf_session(
  const DmaBufRing& ring, mln_vulkan_device device,
  mln_render_session_object& session
) -> mln_status;

auto dma_buf_attach_start(
  mln_map map, const mln_dma_buf_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
