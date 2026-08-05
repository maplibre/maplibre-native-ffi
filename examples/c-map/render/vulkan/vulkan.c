// The Vulkan render target: a shared instance/device/queue bridged to the C API
// through the Vulkan context descriptor, plus a fullscreen-triangle compositor
// over a window swapchain.

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#include "../../diagnostics.h"
#include "../../render_target.h"
#include "../../types.h"
#include "../../util.h"
#include "../render.h"
#include "commands.h"
#include "context.h"
#include "pipeline.h"
#include "swapchain.h"
#include "util.h"

static constexpr VkFormat borrowed_image_format = VK_FORMAT_R8G8B8A8_UNORM;

static mln_vulkan_context_descriptor vulkan_context_descriptor(
  const vulkan_context* context
) {
  return (mln_vulkan_context_descriptor){
    .size = sizeof(mln_vulkan_context_descriptor),
    .instance = context->instance,
    .physical_device = context->physical_device,
    .device = context->device,
    .graphics_queue = context->queue,
    .graphics_queue_family_index = context->queue_family_index,
    .get_instance_proc_addr = (void*)vkGetInstanceProcAddr,
    .get_device_proc_addr = (void*)vkGetDeviceProcAddr,
  };
}

/// The compositor: context, swapchain, pipeline, and command state that
/// sample a map-rendered image view into the window swapchain.
typedef struct vulkan_compositor {
  vulkan_context context;
  vulkan_swapchain swapchain;
  vulkan_pipeline pipeline;
  vulkan_commands commands;
  viewport current_viewport;
  bool swapchain_stale;
} vulkan_compositor;

static void vulkan_compositor_deinit(vulkan_compositor* compositor) {
  vulkan_context_wait_idle(&compositor->context);
  vulkan_commands_deinit(&compositor->commands, compositor->context.device);
  vulkan_swapchain_deinit(&compositor->swapchain, compositor->context.device);
  vulkan_pipeline_deinit(&compositor->pipeline, compositor->context.device);
  vulkan_context_deinit(&compositor->context);
}

static app_error vulkan_compositor_create(
  vulkan_compositor* compositor, SDL_Window* window, viewport current_viewport
) {
  MAP_TRY(vulkan_context_init(&compositor->context, window));
  MAP_TRY(vulkan_swapchain_init(
    &compositor->swapchain, &compositor->context, current_viewport,
    VK_NULL_HANDLE
  ));
  MAP_TRY(vulkan_pipeline_init(
    &compositor->pipeline, compositor->context.device,
    compositor->swapchain.format
  ));
  MAP_TRY(vulkan_swapchain_create_framebuffers(
    &compositor->swapchain, compositor->context.device,
    compositor->pipeline.render_pass
  ));
  MAP_TRY(vulkan_commands_init(
    &compositor->commands, compositor->context.device,
    compositor->context.queue_family_index
  ));
  return vulkan_commands_create_present_semaphores(
    &compositor->commands, compositor->context.device,
    compositor->swapchain.image_count
  );
}

static app_error vulkan_compositor_init(
  vulkan_compositor* compositor, SDL_Window* window, viewport current_viewport
) {
  *compositor = (vulkan_compositor){};
  compositor->current_viewport = current_viewport;
  const app_error error =
    vulkan_compositor_create(compositor, window, current_viewport);
  if (error != APP_OK) {
    vulkan_compositor_deinit(compositor);
  }
  return error;
}

/// Notes a resized window without touching the swapchain. The compositor only
/// presents when a map frame is ready, so destroying the swapchain here would
/// blank the window until the map renders at the new extent.
static void vulkan_compositor_resize(
  vulkan_compositor* compositor, viewport current_viewport
) {
  compositor->current_viewport = current_viewport;
  compositor->swapchain_stale = true;
}

static app_error vulkan_compositor_recreate_swapchain(
  vulkan_compositor* compositor
) {
  vulkan_context_wait_idle(&compositor->context);
  // Create the replacement naming the retired swapchain as oldSwapchain before
  // destroying it: on MoltenVK, destroying first leaves presents that succeed
  // but reach no drawable the window shows.
  vulkan_swapchain previous = compositor->swapchain;
  const VkFormat previous_format = previous.format;
  const app_error error = vulkan_swapchain_init(
    &compositor->swapchain, &compositor->context, compositor->current_viewport,
    previous.handle
  );
  vulkan_swapchain_deinit(&previous, compositor->context.device);
  MAP_TRY(error);

  if (compositor->swapchain.format != previous_format) {
    vulkan_pipeline_deinit(&compositor->pipeline, compositor->context.device);
    MAP_TRY(vulkan_pipeline_init(
      &compositor->pipeline, compositor->context.device,
      compositor->swapchain.format
    ));
  }
  MAP_TRY(vulkan_swapchain_create_framebuffers(
    &compositor->swapchain, compositor->context.device,
    compositor->pipeline.render_pass
  ));
  vulkan_commands_destroy_present_semaphores(
    &compositor->commands, compositor->context.device
  );
  return vulkan_commands_create_present_semaphores(
    &compositor->commands, compositor->context.device,
    compositor->swapchain.image_count
  );
}

static app_error vulkan_compositor_wait_for_frame(
  vulkan_compositor* compositor
) {
  return vulkan_commands_wait_for_frame_fence(
    &compositor->commands, compositor->context.device
  );
}

/// Samples image_view into the next swapchain image. Reports false without
/// presenting when the swapchain is out of date; the next resize rebuilds it.
static app_error vulkan_compositor_present_image_view(
  vulkan_compositor* compositor, VkImageView image_view, bool* out_presented
) {
  *out_presented = false;
  MAP_TRY(vulkan_compositor_wait_for_frame(compositor));

  if (compositor->swapchain_stale) {
    MAP_TRY(vulkan_compositor_recreate_swapchain(compositor));
    compositor->swapchain_stale = false;
  }

  // Must follow the fence wait, so no in-flight command reads the descriptor
  // set, and the swapchain replacement, which can rebuild the pipeline.
  if (image_view != compositor->pipeline.descriptor_image_view) {
    vulkan_pipeline_update_descriptor(
      &compositor->pipeline, compositor->context.device, image_view
    );
  }

  uint32_t image_index = 0;
  const VkResult acquire = vkAcquireNextImageKHR(
    compositor->context.device, compositor->swapchain.handle, UINT64_MAX,
    compositor->commands.image_available, VK_NULL_HANDLE, &image_index
  );
  if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
    compositor->swapchain_stale = true;
    return APP_OK;
  }
  if (acquire == VK_SUBOPTIMAL_KHR) {
    // Still presentable, but the surface has moved on.
    compositor->swapchain_stale = true;
  }
  MAP_TRY(expect_vk_or_suboptimal(acquire));
  MAP_TRY(vulkan_commands_reset_fence(
    &compositor->commands, compositor->context.device
  ));

  MAP_TRY(vulkan_commands_record(
    &compositor->commands, &compositor->swapchain, &compositor->pipeline,
    image_index
  ));
  MAP_TRY(vulkan_commands_submit(
    &compositor->commands, compositor->context.queue, image_index
  ));

  const VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &compositor->commands.render_finished[image_index],
    .swapchainCount = 1,
    .pSwapchains = &compositor->swapchain.handle,
    .pImageIndices = &image_index,
  };
  const VkResult present =
    vkQueuePresentKHR(compositor->context.queue, &present_info);
  if (present == VK_ERROR_OUT_OF_DATE_KHR) {
    // Nothing reached the screen, but the sampling pass was submitted; wait it
    // out before the caller releases its frame.
    compositor->swapchain_stale = true;
    (void)vulkan_compositor_wait_for_frame(compositor);
    return APP_OK;
  }
  if (present != VK_SUCCESS && present != VK_SUBOPTIMAL_KHR) {
    (void)vulkan_compositor_wait_for_frame(compositor);
    return expect_vk(present);
  }
  if (present == VK_SUBOPTIMAL_KHR) {
    compositor->swapchain_stale = true;
  }
  *out_presented = true;
  return APP_OK;
}

/// The caller-owned image handed to a borrowed-texture session.
typedef struct borrowed_image {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
} borrowed_image;

static void borrowed_image_deinit(borrowed_image* image, VkDevice device) {
  if (image->view != VK_NULL_HANDLE) {
    vkDestroyImageView(device, image->view, nullptr);
  }
  if (image->image != VK_NULL_HANDLE) {
    vkDestroyImage(device, image->image, nullptr);
  }
  if (image->memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, image->memory, nullptr);
  }
  *image = (borrowed_image){};
}

static app_error find_memory_type(
  VkPhysicalDevice physical_device, uint32_t type_bits,
  VkMemoryPropertyFlags properties, uint32_t* out_index
) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
  for (uint32_t index = 0; index < memory_properties.memoryTypeCount;
       index += 1) {
    if ((type_bits & (1u << index)) == 0) {
      continue;
    }
    if (
      (memory_properties.memoryTypes[index].propertyFlags & properties) ==
      properties
    ) {
      *out_index = index;
      return APP_OK;
    }
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

static app_error borrowed_image_create(
  borrowed_image* image, const vulkan_context* context,
  viewport current_viewport
) {
  const VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = borrowed_image_format,
    .extent =
      {
        .width = current_viewport.physical_width,
        .height = current_viewport.physical_height,
        .depth = 1,
      },
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  MAP_TRY(expect_vk(
    vkCreateImage(context->device, &image_info, nullptr, &image->image)
  ));

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(context->device, image->image, &requirements);
  uint32_t memory_type_index = 0;
  MAP_TRY(find_memory_type(
    context->physical_device, requirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type_index
  ));
  const VkMemoryAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    .memoryTypeIndex = memory_type_index,
  };
  MAP_TRY(expect_vk(
    vkAllocateMemory(context->device, &allocate_info, nullptr, &image->memory)
  ));
  MAP_TRY(expect_vk(
    vkBindImageMemory(context->device, image->image, image->memory, 0)
  ));

  return vulkan_create_image_view(
    context->device, image->image, borrowed_image_format, &image->view
  );
}

static app_error borrowed_image_init(
  borrowed_image* image, const vulkan_context* context,
  viewport current_viewport
) {
  *image = (borrowed_image){};
  const app_error error =
    borrowed_image_create(image, context, current_viewport);
  if (error != APP_OK) {
    borrowed_image_deinit(image, context->device);
  }
  return error;
}

struct render_target {
  render_target_mode mode;
  render_session session;
  union {
    struct {
      vulkan_compositor compositor;
      mln_vulkan_owned_texture_frame pending_frame;
      bool has_pending_frame;
    } owned;
    struct {
      vulkan_compositor compositor;
      borrowed_image image;
    } borrowed;
    struct {
      vulkan_context context;
    } surface;
  } as;
};

uint32_t render_target_backend_flag(void) {
  return MLN_RENDER_BACKEND_FLAG_VULKAN;
}

void render_target_apply_sdl_hints(void) {}

app_error render_target_configure_video(void) { return APP_OK; }

SDL_WindowFlags render_target_window_flags(void) { return SDL_WINDOW_VULKAN; }

void* render_target_frame_scope_open(void) { return nullptr; }

void render_target_frame_scope_close(void* scope) { (void)scope; }

app_error render_target_init(
  render_target** out_target, SDL_Window* window, viewport current_viewport,
  render_target_mode mode
) {
  render_target* target = calloc(1, sizeof(render_target));
  if (target == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  target->mode = mode;

  app_error error = APP_OK;
  switch (mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      error = vulkan_compositor_init(
        &target->as.owned.compositor, window, current_viewport
      );
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      error = vulkan_compositor_init(
        &target->as.borrowed.compositor, window, current_viewport
      );
      if (error == APP_OK) {
        error = borrowed_image_init(
          &target->as.borrowed.image, &target->as.borrowed.compositor.context,
          current_viewport
        );
        if (error != APP_OK) {
          vulkan_compositor_deinit(&target->as.borrowed.compositor);
        }
      }
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      error = vulkan_context_init(&target->as.surface.context, window);
      break;
  }
  if (error != APP_OK) {
    free(target);
    return error;
  }
  *out_target = target;
  return APP_OK;
}

static void release_pending_frame(render_target* target) {
  if (!target->as.owned.has_pending_frame) {
    return;
  }
  const mln_status status = mln_vulkan_owned_texture_release_frame(
    target->session.handle, &target->as.owned.pending_frame
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("Vulkan texture release failed", status);
  }
  target->as.owned.has_pending_frame = false;
}

static mln_vulkan_borrowed_texture_descriptor borrowed_image_descriptor(
  render_target* target, viewport current_viewport
) {
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.extent = render_target_extent(current_viewport);
  descriptor.physical_width = current_viewport.physical_width;
  descriptor.physical_height = current_viewport.physical_height;
  descriptor.context =
    vulkan_context_descriptor(&target->as.borrowed.compositor.context);
  descriptor.image = target->as.borrowed.image.image;
  descriptor.image_view = target->as.borrowed.image.view;
  descriptor.format = borrowed_image_format;
  descriptor.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  descriptor.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return descriptor;
}

app_error render_target_attach(
  render_target* target, mln_map map, viewport current_viewport
) {
  mln_render_session session = MLN_HANDLE_NULL;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE: {
      mln_vulkan_owned_texture_descriptor descriptor =
        mln_vulkan_owned_texture_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        vulkan_context_descriptor(&target->as.owned.compositor.context);
      const mln_status status =
        mln_vulkan_owned_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Vulkan texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      const mln_vulkan_borrowed_texture_descriptor descriptor =
        borrowed_image_descriptor(target, current_viewport);
      const mln_status status =
        mln_vulkan_borrowed_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Vulkan borrowed texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE: {
      mln_vulkan_surface_descriptor descriptor =
        mln_vulkan_surface_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        vulkan_context_descriptor(&target->as.surface.context);
      descriptor.surface = target->as.surface.context.surface;
      const mln_status status =
        mln_vulkan_surface_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Vulkan surface attach failed", status);
        return APP_ERROR_SURFACE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_SURFACE, .handle = session};
      return APP_OK;
    }
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

void render_target_deinit(render_target* target) {
  if (target == nullptr) {
    return;
  }
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      vulkan_context_wait_idle(&target->as.owned.compositor.context);
      release_pending_frame(target);
      render_session_close(&target->session);
      vulkan_compositor_deinit(&target->as.owned.compositor);
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      vulkan_context_wait_idle(&target->as.borrowed.compositor.context);
      render_session_close(&target->session);
      borrowed_image_deinit(
        &target->as.borrowed.image,
        target->as.borrowed.compositor.context.device
      );
      vulkan_compositor_deinit(&target->as.borrowed.compositor);
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      vulkan_context_wait_idle(&target->as.surface.context);
      render_session_close(&target->session);
      vulkan_context_deinit(&target->as.surface.context);
      break;
  }
  free(target);
}

/// Follows a resized window in borrowed-texture mode: allocates an image at
/// the new size and hands it to the live session, which stays attached.
static app_error resize_borrowed(
  render_target* target, viewport current_viewport
) {
  if (target->session.kind != RENDER_SESSION_TEXTURE) {
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  vulkan_context_wait_idle(&target->as.borrowed.compositor.context);
  vulkan_compositor_resize(&target->as.borrowed.compositor, current_viewport);

  borrowed_image previous = target->as.borrowed.image;
  borrowed_image replacement;
  MAP_TRY(borrowed_image_init(
    &replacement, &target->as.borrowed.compositor.context, current_viewport
  ));
  target->as.borrowed.image = replacement;
  const mln_vulkan_borrowed_texture_descriptor descriptor =
    borrowed_image_descriptor(target, current_viewport);
  const mln_status status =
    mln_vulkan_borrowed_texture_set_target(target->session.handle, &descriptor);
  if (status != MLN_STATUS_OK) {
    // The session may have taken the replacement before failing, so detach
    // before either image is released.
    mln_render_session_detach(target->session.handle);
    diagnostics_log_status("Vulkan borrowed texture set target failed", status);
    target->as.borrowed.image = previous;
    borrowed_image_deinit(
      &replacement, target->as.borrowed.compositor.context.device
    );
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  // Released only once the session has taken the replacement.
  borrowed_image_deinit(
    &previous, target->as.borrowed.compositor.context.device
  );
  return APP_OK;
}

app_error render_target_resize(
  render_target* target, viewport current_viewport
) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      vulkan_context_wait_idle(&target->as.owned.compositor.context);
      release_pending_frame(target);
      vulkan_compositor_resize(&target->as.owned.compositor, current_viewport);
      return render_session_resize(&target->session, current_viewport);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return resize_borrowed(target, current_viewport);
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return render_session_resize(&target->session, current_viewport);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

app_error render_target_finish_frame(render_target* target) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      if (!target->as.owned.has_pending_frame) {
        return APP_OK;
      }
      MAP_TRY(vulkan_compositor_wait_for_frame(&target->as.owned.compositor));
      release_pending_frame(target);
      return APP_OK;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return vulkan_compositor_wait_for_frame(&target->as.borrowed.compositor);
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return APP_OK;
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

static app_error render_update_owned(
  render_target* target, bool* out_rendered
) {
  bool rendered = false;
  MAP_TRY(render_session_render_update(&target->session, &rendered));
  if (!rendered) {
    return APP_OK;
  }

  mln_vulkan_owned_texture_frame frame = {.size = sizeof(frame)};
  const mln_status status =
    mln_vulkan_owned_texture_acquire_frame(target->session.handle, &frame);
  if (status == MLN_STATUS_INVALID_STATE) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("Vulkan texture acquire failed", status);
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }

  bool presented = false;
  const app_error error = vulkan_compositor_present_image_view(
    &target->as.owned.compositor, frame.image_view, &presented
  );
  if (error != APP_OK || !presented) {
    const mln_status release_status =
      mln_vulkan_owned_texture_release_frame(target->session.handle, &frame);
    if (release_status != MLN_STATUS_OK) {
      diagnostics_log_status("Vulkan texture release failed", release_status);
    }
    return error;
  }

  // The frame stays acquired until the compositor's fence proves the sampling
  // pass finished; finish_frame releases it.
  target->as.owned.pending_frame = frame;
  target->as.owned.has_pending_frame = true;
  *out_rendered = true;
  return APP_OK;
}

app_error render_target_render_update(
  render_target* target, [[maybe_unused]] viewport current_viewport,
  bool* out_rendered
) {
  *out_rendered = false;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return render_update_owned(target, out_rendered);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      bool rendered = false;
      MAP_TRY(render_session_render_update(&target->session, &rendered));
      if (!rendered) {
        return APP_OK;
      }
      return vulkan_compositor_present_image_view(
        &target->as.borrowed.compositor, target->as.borrowed.image.view,
        out_rendered
      );
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return render_session_render_update(&target->session, out_rendered);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}
