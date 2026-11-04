#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__linux__)
#include <linux/memfd.h>
#include <linux/udmabuf.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#endif

#include "platform/dma_buf/allocator.hpp"
#include "render/dma_buf_session.hpp"

namespace mln::platform::dma_buf {
namespace {

// libgbm's declarations, which the library loads at run time so that it runs
// on a system without GBM.
struct gbm_device;
struct gbm_bo;
constexpr auto gbm_bo_use_rendering = uint32_t{1U << 2U};

struct GbmFunctions {
  gbm_device* (*create_device)(int) = nullptr;
  void (*device_destroy)(gbm_device*) = nullptr;
  int (*modifier_plane_count)(gbm_device*, uint32_t, uint64_t) = nullptr;
  gbm_bo* (*create_with_modifiers)(
    gbm_device*, uint32_t, uint32_t, uint32_t, const uint64_t*, unsigned int
  ) = nullptr;
  gbm_bo* (*create_with_modifiers2)(
    gbm_device*, uint32_t, uint32_t, uint32_t, const uint64_t*, unsigned int,
    uint32_t
  ) = nullptr;
  int (*plane_count)(gbm_bo*) = nullptr;
  int (*fd)(gbm_bo*) = nullptr;
  uint64_t (*modifier)(gbm_bo*) = nullptr;
  uint32_t (*offset)(gbm_bo*, int) = nullptr;
  uint32_t (*stride)(gbm_bo*, int) = nullptr;
  void (*destroy)(gbm_bo*) = nullptr;
};

// The functions, or null when libgbm is missing or lacks one. The handle stays
// open for the rest of the process.
auto gbm_functions() -> const GbmFunctions* {
  static const auto* const functions = []() -> const GbmFunctions* {
    auto* const library = dlopen("libgbm.so.1", RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) return nullptr;
    static auto loaded = GbmFunctions{};
    const auto load = [library](auto& out, const char* name) -> bool {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      out = reinterpret_cast<std::remove_reference_t<decltype(out)>>(
        dlsym(library, name)
      );
      return out != nullptr;
    };
    // gbm_bo_create_with_modifiers2, which takes the buffer's usage, came in
    // Mesa 21.3.
    load(loaded.create_with_modifiers2, "gbm_bo_create_with_modifiers2");
    if (
      !load(loaded.create_device, "gbm_create_device") ||
      !load(loaded.device_destroy, "gbm_device_destroy") ||
      !load(
        loaded.modifier_plane_count,
        "gbm_device_get_format_modifier_plane_count"
      ) ||
      !load(loaded.create_with_modifiers, "gbm_bo_create_with_modifiers") ||
      !load(loaded.plane_count, "gbm_bo_get_plane_count") ||
      !load(loaded.fd, "gbm_bo_get_fd") ||
      !load(loaded.modifier, "gbm_bo_get_modifier") ||
      !load(loaded.offset, "gbm_bo_get_offset") ||
      !load(loaded.stride, "gbm_bo_get_stride_for_plane") ||
      !load(loaded.destroy, "gbm_bo_destroy")
    ) {
      return nullptr;
    }
    return &loaded;
  }();
  return functions;
}

class GbmAllocator final : public Allocator {
 public:
  GbmAllocator(const GbmFunctions& gbm, core::OwnedFd node, gbm_device* device)
      : gbm_(gbm), node_(std::move(node)), device_(device) {}
  GbmAllocator(const GbmAllocator&) = delete;
  auto operator=(const GbmAllocator&) -> GbmAllocator& = delete;
  GbmAllocator(GbmAllocator&&) = delete;
  auto operator=(GbmAllocator&&) -> GbmAllocator& = delete;
  ~GbmAllocator() override { gbm_.device_destroy(device_); }

  [[nodiscard]] auto allocates(uint32_t fourcc, uint64_t modifier) const
    -> bool override {
    return gbm_.modifier_plane_count(device_, fourcc, modifier) == 1;
  }

  [[nodiscard]] auto allocate(
    uint32_t width, uint32_t height, uint32_t fourcc,
    const std::vector<uint64_t>& modifiers
  ) const -> Buffer override {
    const auto count = static_cast<unsigned int>(modifiers.size());
    auto* const object =
      gbm_.create_with_modifiers2 != nullptr
        ? gbm_.create_with_modifiers2(
            device_, width, height, fourcc, modifiers.data(), count,
            gbm_bo_use_rendering
          )
        : gbm_.create_with_modifiers(
            device_, width, height, fourcc, modifiers.data(), count
          );
    if (object == nullptr) {
      throw std::runtime_error("GBM can't allocate the dmabuf");
    }
    // The dmabuf keeps the buffer's memory once the buffer object is gone.
    auto buffer = Buffer{
      .fd = core::OwnedFd{gbm_.plane_count(object) == 1 ? gbm_.fd(object) : -1},
      .modifier = gbm_.modifier(object),
      .offset = gbm_.offset(object, 0),
      .stride = gbm_.stride(object, 0),
    };
    gbm_.destroy(object);
    if (buffer.fd.get() < 0) {
      throw std::runtime_error("GBM can't export the dmabuf");
    }
    return buffer;
  }

 private:
  const GbmFunctions& gbm_;
  core::OwnedFd node_;
  gbm_device* device_;
};

#if defined(__linux__)
// glibc declares file seals from 2.20 on, after the glibc floor.
#ifndef F_ADD_SEALS
#define F_ADD_SEALS 1033
#endif
#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK 0x0002
#endif

// Rows of 256 bytes suit the GPUs that import linear buffers.
constexpr auto udmabuf_row_alignment = uint32_t{256};

class UdmabufAllocator final : public Allocator {
 public:
  explicit UdmabufAllocator(core::OwnedFd device)
      : device_(std::move(device)) {}

  [[nodiscard]] auto allocates(uint32_t fourcc, uint64_t modifier) const
    -> bool override {
    static_cast<void>(fourcc);
    return modifier == core::drm_format_mod_linear;
  }

  [[nodiscard]] auto allocate(
    uint32_t width, uint32_t height, uint32_t fourcc,
    const std::vector<uint64_t>& modifiers
  ) const -> Buffer override {
    static_cast<void>(fourcc);
    static_cast<void>(modifiers);
    const auto stride = ((width * 4) + udmabuf_row_alignment - 1) /
                        udmabuf_row_alignment * udmabuf_row_alignment;
    const auto page = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    const auto size =
      ((static_cast<std::size_t>(stride) * height) + page - 1) / page * page;
    // memfd_create came in glibc 2.27, after the glibc floor.
    const auto memory = core::OwnedFd{static_cast<int>(
      syscall(SYS_memfd_create, "mln-dma-buf", MFD_ALLOW_SEALING | MFD_CLOEXEC)
    )};
    if (
      memory.get() < 0 ||
      ftruncate(memory.get(), static_cast<off_t>(size)) != 0 ||
      // udmabuf takes only memory that can't shrink.
      fcntl(memory.get(), F_ADD_SEALS, F_SEAL_SHRINK) != 0
    ) {
      throw std::runtime_error("allocating the dmabuf's memory failed");
    }
    auto create = udmabuf_create{
      .memfd = static_cast<uint32_t>(memory.get()),
      .flags = UDMABUF_FLAGS_CLOEXEC,
      .offset = 0,
      .size = size,
    };
    auto buffer = Buffer{
      .fd = core::OwnedFd{ioctl(device_.get(), UDMABUF_CREATE, &create)},
      .modifier = core::drm_format_mod_linear,
      .offset = 0,
      .stride = stride,
    };
    if (buffer.fd.get() < 0) {
      throw std::runtime_error("udmabuf can't share the dmabuf's memory");
    }
    return buffer;
  }

 private:
  core::OwnedFd device_;
};
#endif

}  // namespace

auto gbm_allocator(const std::string& path) -> std::unique_ptr<Allocator> {
  const auto* const gbm = gbm_functions();
  if (gbm == nullptr) return nullptr;
  auto node = core::OwnedFd{open(path.c_str(), O_RDWR | O_CLOEXEC)};
  if (node.get() < 0) return nullptr;
  auto* const device = gbm->create_device(node.get());
  if (device == nullptr) return nullptr;
  return std::make_unique<GbmAllocator>(*gbm, std::move(node), device);
}

auto udmabuf_allocator() -> std::unique_ptr<Allocator> {
#if defined(__linux__)
  auto device = core::OwnedFd{open("/dev/udmabuf", O_RDWR | O_CLOEXEC)};
  if (device.get() < 0) return nullptr;
  return std::make_unique<UdmabufAllocator>(std::move(device));
#else
  return nullptr;
#endif
}

}  // namespace mln::platform::dma_buf
