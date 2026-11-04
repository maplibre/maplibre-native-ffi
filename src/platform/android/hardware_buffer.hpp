#pragma once

#include <cstdint>
#include <memory>

#include <android/hardware_buffer.h>

namespace mln::platform::android {

// The AHardwareBuffer functions, which API level 26 added. The library also
// runs on older versions, so it looks them up at run time.
struct HardwareBufferFunctions {
  int (*allocate)(const AHardwareBuffer_Desc*, AHardwareBuffer**) = nullptr;
  void (*release)(AHardwareBuffer*) = nullptr;
};

// The functions, or null below API level 26.
auto hardware_buffer_functions() noexcept -> const HardwareBufferFunctions*;

// A hardware buffer that the library allocated, which it releases with the
// last copy.
using HardwareBuffer = std::shared_ptr<AHardwareBuffer>;

// Allocates an RGBA8888 buffer that the GPU renders into and samples, with
// the extra `usage` bits. Throws when Android refuses it, or when Android is
// older than API level 26.
auto allocate_render_buffer(uint32_t width, uint32_t height, uint64_t usage)
  -> HardwareBuffer;

}  // namespace mln::platform::android
