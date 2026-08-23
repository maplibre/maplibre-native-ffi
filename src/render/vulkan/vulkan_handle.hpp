#pragma once

#include <cstdint>
#include <type_traits>

#include <vulkan/vulkan_core.h>

#include "maplibre_native_c/render_target.h"

namespace mln::core {

template <typename Handle>
[[nodiscard]] auto vulkan_handle_from_abi(
  mln_vulkan_non_dispatchable_handle handle
) noexcept -> Handle {
  static_assert(sizeof(Handle) <= sizeof(handle));
#if VK_USE_64_BIT_PTR_DEFINES
  static_assert(std::is_pointer_v<Handle>);
  return reinterpret_cast<Handle>(static_cast<uintptr_t>(handle));
#else
  static_assert(std::is_integral_v<Handle>);
  return static_cast<Handle>(handle);
#endif
}

template <typename Handle>
[[nodiscard]] auto vulkan_handle_to_abi(Handle handle) noexcept
  -> mln_vulkan_non_dispatchable_handle {
  static_assert(sizeof(Handle) <= sizeof(mln_vulkan_non_dispatchable_handle));
#if VK_USE_64_BIT_PTR_DEFINES
  static_assert(std::is_pointer_v<Handle>);
  return static_cast<mln_vulkan_non_dispatchable_handle>(
    reinterpret_cast<uintptr_t>(handle)
  );
#else
  static_assert(std::is_integral_v<Handle>);
  return static_cast<mln_vulkan_non_dispatchable_handle>(handle);
#endif
}

}  // namespace mln::core
