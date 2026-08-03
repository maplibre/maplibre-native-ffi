// Small expectation helpers shared by the Vulkan backend files.

#ifndef C_MAP_RENDER_VULKAN_UTIL_H
#define C_MAP_RENDER_VULKAN_UTIL_H

#include <vulkan/vulkan.h>

#include "../../types.h"

/// Returns from the enclosing function when a fallible step fails, so the
/// backend's setup sequences read as straight lines.
#define MAP_TRY(expr)                    \
  do {                                   \
    const app_error try_error_ = (expr); \
    if (try_error_ != APP_OK) {          \
      return try_error_;                 \
    }                                    \
  } while (0)

[[nodiscard]] app_error expect_vk(VkResult result);
[[nodiscard]] app_error expect_vk_or_suboptimal(VkResult result);
[[nodiscard]] app_error expect_sdl(bool ok);

#endif  // C_MAP_RENDER_VULKAN_UTIL_H
