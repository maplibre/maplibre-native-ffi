// The Vulkan render target: a shared instance and device bridged to the C API
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

/// Releases the compositor once the session detached, so nothing else submits.
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
  SDL_LockMutex(compositor->context.queue_mutex);
  const VkResult idle = vkQueueWaitIdle(compositor->context.queue);
  SDL_UnlockMutex(compositor->context.queue_mutex);
  MAP_TRY(expect_vk(idle));
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
  SDL_LockMutex(compositor->context.queue_mutex);
  const app_error submitted = vulkan_commands_submit(
    &compositor->commands, compositor->context.queue, image_index
  );
  SDL_UnlockMutex(compositor->context.queue_mutex);
  MAP_TRY(submitted);

  const VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &compositor->commands.render_finished[image_index],
    .swapchainCount = 1,
    .pSwapchains = &compositor->swapchain.handle,
    .pImageIndices = &image_index,
  };
  SDL_LockMutex(compositor->context.queue_mutex);
  const VkResult present =
    vkQueuePresentKHR(compositor->context.queue, &present_info);
  SDL_UnlockMutex(compositor->context.queue_mutex);
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
      /// The newest frame, held until a newer one replaces it.
      mln_acquired_frame held;
    } owned;
    struct {
      vulkan_compositor compositor;
      /// The image the compositor samples.
      borrowed_image image;
      texture_replacements replacements;
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

render_session* render_target_session(render_target* target) {
  return &target->session;
}

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

static mln_vulkan_borrowed_texture_descriptor borrowed_image_descriptor(
  render_target* target, const borrowed_image* image, viewport current_viewport
) {
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.extent = render_target_extent(current_viewport);
  descriptor.physical_width = current_viewport.physical_width;
  descriptor.physical_height = current_viewport.physical_height;
  descriptor.context =
    vulkan_context_descriptor(&target->as.borrowed.compositor.context);
  descriptor.image = vulkan_image_to_abi(image->image);
  descriptor.image_view = vulkan_image_view_to_abi(image->view);
  descriptor.format = borrowed_image_format;
  descriptor.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  descriptor.final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return descriptor;
}

static const vulkan_context* target_context(const render_target* target) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return &target->as.owned.compositor.context;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return &target->as.borrowed.compositor.context;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return &target->as.surface.context;
  }
  return nullptr;
}

app_error render_target_attach(
  render_target* target, mln_map map, viewport current_viewport
) {
  awaited_completion attached;
  mln_completion completion;
  MAP_TRY(awaited_completion_init(&attached, &completion));
  // The core worker submits to the host's queue, so it takes the host's queue
  // lock around each call on it.
  mln_render_session_attach_options options =
    render_session_attach_options(target->mode, MLN_RENDER_DRIVER_CORE_WORKER);
  options.queue_lock = vulkan_context_queue_lock(target_context(target));
  mln_render_session session = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_status status = MLN_STATUS_INVALID_STATE;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE: {
      mln_vulkan_owned_texture_descriptor descriptor =
        mln_vulkan_owned_texture_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        vulkan_context_descriptor(&target->as.owned.compositor.context);
      status = mln_map_attach_vulkan_owned_texture(
        map, &descriptor, &options, &session, &completion, &diagnostic
      );
      break;
    }
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      const mln_vulkan_borrowed_texture_descriptor descriptor =
        borrowed_image_descriptor(
          target, &target->as.borrowed.image, current_viewport
        );
      status = mln_map_attach_vulkan_borrowed_texture(
        map, &descriptor, &options, &session, &completion, &diagnostic
      );
      break;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE: {
      mln_vulkan_surface_descriptor descriptor =
        mln_vulkan_surface_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        vulkan_context_descriptor(&target->as.surface.context);
      descriptor.surface =
        vulkan_surface_to_abi(target->as.surface.context.surface);
      status = mln_map_attach_vulkan_surface(
        map, &descriptor, &options, &session, &completion, &diagnostic
      );
      break;
    }
  }
  return render_session_finish_attach(
    &target->session, session, map, &options, target->mode, &attached, status,
    &diagnostic
  );
}

static void destroy_replacement(void* replacement, VkDevice device) {
  borrowed_image_deinit(replacement, device);
  free(replacement);
}

void render_target_deinit(render_target* target) {
  if (target == nullptr) {
    return;
  }
  // The compositor waited for its reads of each frame, so the session may
  // detach, and once it has, nothing else submits.
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      render_session_release_frame(&target->as.owned.held);
      render_session_close(&target->session);
      vulkan_compositor_deinit(&target->as.owned.compositor);
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      vulkan_compositor* compositor = &target->as.borrowed.compositor;
      render_session_close(&target->session);
      void* replacement = nullptr;
      texture_replacements_take_any(
        &target->as.borrowed.replacements, &replacement
      );
      while (replacement != nullptr) {
        destroy_replacement(replacement, compositor->context.device);
        texture_replacements_take_any(
          &target->as.borrowed.replacements, &replacement
        );
      }
      borrowed_image_deinit(
        &target->as.borrowed.image, compositor->context.device
      );
      vulkan_compositor_deinit(compositor);
      break;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      render_session_close(&target->session);
      vulkan_context_wait_idle(&target->as.surface.context);
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
  vulkan_compositor* compositor = &target->as.borrowed.compositor;
  vulkan_compositor_resize(compositor, current_viewport);
  borrowed_image* replacement = calloc(1, sizeof(borrowed_image));
  if (replacement == nullptr) return APP_ERROR_RESIZE_FAILED;
  app_error error =
    borrowed_image_init(replacement, &compositor->context, current_viewport);
  if (error != APP_OK) {
    free(replacement);
    return error;
  }
  mln_completion completion;
  texture_replacement* entry =
    texture_replacement_begin(replacement, &completion);
  mln_status status = MLN_STATUS_INVALID_STATE;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  if (entry != nullptr) {
    const mln_vulkan_borrowed_texture_descriptor descriptor =
      borrowed_image_descriptor(target, replacement, current_viewport);
    status = mln_render_session_set_vulkan_borrowed_texture_target(
      target->session.handle, &descriptor, &completion, &diagnostic
    );
    texture_replacements_queue(
      &target->as.borrowed.replacements, entry, status
    );
  }
  if (status != MLN_STATUS_OK) {
    destroy_replacement(replacement, compositor->context.device);
    diagnostics_log_status(
      "Vulkan borrowed texture set target failed", status, &diagnostic
    );
    return APP_ERROR_RESIZE_FAILED;
  }
  return render_session_resize_map(&target->session, current_viewport);
}

app_error render_target_resize(
  render_target* target, viewport current_viewport
) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      vulkan_compositor_resize(&target->as.owned.compositor, current_viewport);
      // A session resizes only while the host holds none of its frames.
      render_session_release_frame(&target->as.owned.held);
      return render_session_resize(&target->session, current_viewport);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return resize_borrowed(target, current_viewport);
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return render_session_resize(&target->session, current_viewport);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

/// Switches the compositor to each replacement a rendered frame has drawn
/// into, destroying the image it retires. The session stopped rendering into
/// that image when the replacement completed, and the compositor waited for
/// its own reads.
app_error render_target_show_replacements(render_target* target) {
  vulkan_compositor* compositor = &target->as.borrowed.compositor;
  while (true) {
    void* replacement = nullptr;
    MAP_TRY(texture_replacements_take_shown(
      &target->as.borrowed.replacements, &target->session, &replacement
    ));
    if (replacement == nullptr) return APP_OK;
    borrowed_image_deinit(
      &target->as.borrowed.image, compositor->context.device
    );
    target->as.borrowed.image = *(borrowed_image*)replacement;
    free(replacement);
  }
}

/// Samples image_view into the window and waits for the sampling pass, so the
/// caller may hand the image back to the session.
static app_error present_image_view(
  vulkan_compositor* compositor, VkImageView image_view, bool* out_presented
) {
  MAP_TRY(
    vulkan_compositor_present_image_view(compositor, image_view, out_presented)
  );
  return vulkan_compositor_wait_for_frame(compositor);
}

static app_error present_owned(render_target* target, bool* out_presented) {
  mln_acquired_frame* held = &target->as.owned.held;
  bool acquired = false;
  MAP_TRY(render_session_acquire_newest(&target->session, held, &acquired));
  if (!acquired) {
    // The window keeps the frame it already shows.
    *out_presented = true;
    return APP_OK;
  }
  MAP_TRY(render_session_require_cpu_complete_producer(
    *held, "Vulkan texture acquire failed"
  ));
  mln_vulkan_owned_texture_frame frame = {.size = sizeof(frame)};
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_acquired_frame_get_vulkan_texture(*held, &frame, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("Vulkan texture access failed", status, &diagnostic);
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }
  return present_image_view(
    &target->as.owned.compositor, vulkan_image_view_from_abi(frame.image_view),
    out_presented
  );
}

app_error render_target_present(
  render_target* target, [[maybe_unused]] viewport current_viewport,
  bool* out_presented
) {
  *out_presented = false;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return present_owned(target, out_presented);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      MAP_TRY(render_target_show_replacements(target));
      return present_image_view(
        &target->as.borrowed.compositor, target->as.borrowed.image.view,
        out_presented
      );
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      *out_presented = true;
      return APP_OK;
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}
