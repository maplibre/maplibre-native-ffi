#include <SDL3/SDL.h>
#include <stdio.h>

#include "util.h"

app_error expect_vk(VkResult result) {
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Vulkan call failed: %d\n", (int)result);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

app_error expect_vk_or_suboptimal(VkResult result) {
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    fprintf(stderr, "Vulkan call failed: %d\n", (int)result);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

app_error expect_sdl(bool ok) {
  if (!ok) {
    fprintf(stderr, "SDL Vulkan call failed: %s\n", SDL_GetError());
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}
