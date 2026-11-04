#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "render/dma_buf_session.hpp"

namespace mln::platform::dma_buf {

// One plane of a dmabuf. Planes in one dmabuf share a file descriptor, which
// the first of them owns.
struct Plane {
  core::OwnedFd owned;
  int fd = -1;
  uint32_t offset = 0;
  uint32_t stride = 0;
};

// A dmabuf that an allocator made, with where each of its planes lies.
struct Buffer {
  uint64_t modifier = 0;
  std::vector<Plane> planes;
};

// Allocates dmabufs for a session to render into.
class Allocator {
 public:
  Allocator() = default;
  Allocator(const Allocator&) = delete;
  auto operator=(const Allocator&) -> Allocator& = delete;
  Allocator(Allocator&&) = delete;
  auto operator=(Allocator&&) -> Allocator& = delete;
  virtual ~Allocator() = default;

  // Whether the allocator can make and share a buffer of `fourcc` with
  // `modifier`, in every plane that the modifier gives it.
  [[nodiscard]] virtual auto allocates(uint32_t fourcc, uint64_t modifier) const
    -> bool = 0;
  // Allocates a buffer of `fourcc` with one of `modifiers`. Throws on failure.
  [[nodiscard]] virtual auto allocate(
    uint32_t width, uint32_t height, uint32_t fourcc,
    const std::vector<uint64_t>& modifiers
  ) const -> Buffer = 0;
};

// Allocates on the GPU that the DRM render node at `path` names, through GBM,
// or returns null when GBM can't open it.
auto gbm_allocator(const std::string& path) -> std::unique_ptr<Allocator>;

// Allocates linear buffers in shared memory through udmabuf, for a renderer
// on the CPU, or returns null when the system has no udmabuf.
auto udmabuf_allocator() -> std::unique_ptr<Allocator>;

}  // namespace mln::platform::dma_buf
