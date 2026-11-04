#include <cstdint>
#include <stdexcept>
#include <string>

#include <android/hardware_buffer.h>
#include <dlfcn.h>

#include "platform/android/hardware_buffer.hpp"

namespace mln::platform::android {

auto hardware_buffer_functions() noexcept -> const HardwareBufferFunctions* {
  static const auto* const functions = []() -> const HardwareBufferFunctions* {
    // libnativewindow is public from API level 26 on, the level that added
    // these functions. The handle stays open for the rest of the process.
    auto* const library = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) return nullptr;
    static auto loaded = HardwareBufferFunctions{};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    loaded.allocate = reinterpret_cast<decltype(loaded.allocate)>(
      dlsym(library, "AHardwareBuffer_allocate")
    );
    loaded.release = reinterpret_cast<decltype(loaded.release)>(
      dlsym(library, "AHardwareBuffer_release")
    );
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    if (loaded.allocate == nullptr || loaded.release == nullptr) {
      return nullptr;
    }
    return &loaded;
  }();
  return functions;
}

auto allocate_render_buffer(uint32_t width, uint32_t height, uint64_t usage)
  -> HardwareBuffer {
  const auto* const functions = hardware_buffer_functions();
  if (functions == nullptr) {
    throw std::runtime_error("Android hardware buffers need API level 26");
  }
  const auto description = AHardwareBuffer_Desc{
    .width = width,
    .height = height,
    .layers = 1,
    .format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
    .usage = AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
             AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | usage,
    .stride = 0,
    .rfu0 = 0,
    .rfu1 = 0,
  };
  AHardwareBuffer* buffer = nullptr;
  const auto result = functions->allocate(&description, &buffer);
  if (result != 0 || buffer == nullptr) {
    throw std::runtime_error(
      "allocating an Android hardware buffer failed with error " +
      std::to_string(result)
    );
  }
  // The deleter runs even when allocating the control block throws.
  return HardwareBuffer{buffer, functions->release};
}

}  // namespace mln::platform::android
