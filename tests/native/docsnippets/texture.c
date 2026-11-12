// Snippets for the docs site's Render to a texture page. They compile
// with the native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

static atomic_bool finished;

static void on_done(void* user_data, const mln_completion_result* result) {
  (void)user_data;
  (void)result;
  atomic_store(&finished, true);
}

static void wait_for_done(void) {
  while (!atomic_exchange(&finished, false)) {
  }
}

static const mln_completion done = {
  .size = sizeof(mln_completion), .callback = on_done
};

static mln_render_session session = MLN_HANDLE_NULL;

// The frame wake from the Render to a window page.
static void on_frame_wake(void* user_data) { (void)user_data; }

// Stand-ins for the host's mutex on its graphics queue.
static void lock_queue(void* user_data) { (void)user_data; }

static void unlock_queue(void* user_data) { (void)user_data; }

// The host's draw, which samples the image view.
static void draw_map(mln_vulkan_non_dispatchable_handle image_view) {
  (void)image_view;
}

static void destroy_textures(
  const mln_vulkan_non_dispatchable_handle images[2],
  const mln_vulkan_non_dispatchable_handle views[2]
) {
  (void)images;
  (void)views;
}

static void draw_frame(void) {
  // #region acquire
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  if (
    mln_render_session_acquire_frame(session, &frame, NULL) != MLN_STATUS_OK ||
    frame == MLN_HANDLE_NULL
  ) {
    return;  // The session failed, or no new frame is ready yet.
  }

  mln_vulkan_texture_frame texture = {.size = sizeof(mln_vulkan_texture_frame)};
  mln_acquired_frame_get_vulkan_texture(frame, &texture, NULL);

  // Sample the texture, and wait for those reads to finish.
  draw_map(texture.image_view);

  const mln_gpu_sync done = {
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_CPU_COMPLETE
  };
  mln_acquired_frame_release(frame, &done, NULL);
  // #endregion acquire
}

void mln_doc_texture(
  mln_map map, mln_vulkan_context_descriptor vulkan,
  const mln_vulkan_non_dispatchable_handle images[2],
  const mln_vulkan_non_dispatchable_handle views[2],
  const mln_vulkan_non_dispatchable_handle new_images[2],
  const mln_vulkan_non_dispatchable_handle new_views[2]
);

void mln_doc_texture(
  mln_map map, mln_vulkan_context_descriptor vulkan,
  const mln_vulkan_non_dispatchable_handle images[2],
  const mln_vulkan_non_dispatchable_handle views[2],
  const mln_vulkan_non_dispatchable_handle new_images[2],
  const mln_vulkan_non_dispatchable_handle new_views[2]
) {
  // #region attach-options
  const mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .frame_wake = {.callback = on_frame_wake},
  };
  // #endregion attach-options

  // #region attach-borrowed
  // The device extensions that you enabled on your VkDevice.
  const mln_buffer_view extensions[] = {
    mln_buffer_view_from_cstr("VK_KHR_swapchain"),
    mln_buffer_view_from_cstr("VK_KHR_external_semaphore_fd"),
  };
  // Your VkDevice and graphics queue.
  const mln_vulkan_device_descriptor host = {
    .size = sizeof(mln_vulkan_device_descriptor),
    .context = vulkan,
    // Only if your renderer also submits to the graphics queue.
    .queue_lock = {.lock = lock_queue, .unlock = unlock_queue},
    .enabled_extensions = extensions,
    .enabled_extension_count = sizeof(extensions) / sizeof(extensions[0]),
  };
  mln_gpu_device device = MLN_HANDLE_NULL;
  mln_gpu_device_wrap_vulkan(&host, &device, NULL);

  // Your VkImage and VkImageView for each texture of the ring.
  const mln_vulkan_borrowed_texture textures[2] = {
    {.image = images[0], .image_view = views[0]},
    {.image = images[1], .image_view = views[1]},
  };
  const mln_render_target ring = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES,
    .data.vulkan_borrowed_textures = {
      // The map's size in logical pixels.
      .logical_size = {.width = 800, .height = 600},
      // The size of your textures in device pixels.
      .physical_width = 1600,
      .physical_height = 1200,
      .textures = textures,
      .texture_count = 2,
      .format = 37,       // VK_FORMAT_R8G8B8A8_UNORM
      .final_layout = 5,  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    },
  };

  mln_map_attach(map, &ring, device, &options, &session, &done, NULL);
  // The session keeps the device until it detaches.
  mln_gpu_device_release(device);
  // #endregion attach-borrowed
  wait_for_done();

  draw_frame();

  // #region resize-borrowed
  // Your new textures, at the new size.
  const mln_vulkan_borrowed_texture new_textures[2] = {
    {.image = new_images[0], .image_view = new_views[0]},
    {.image = new_images[1], .image_view = new_views[1]},
  };
  const mln_render_target resized = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES,
    .data.vulkan_borrowed_textures = {
      .logical_size = {.width = 1024, .height = 768},
      .physical_width = 2048,
      .physical_height = 1536,
      .textures = new_textures,
      .texture_count = 2,
      .format = 37,
      .final_layout = 5,
    },
  };

  mln_render_session_set_target(session, &resized, &done, NULL);
  // #endregion resize-borrowed
  wait_for_done();

  // #region resize-borrowed
  // vkDestroyImageView and vkDestroyImage
  destroy_textures(images, views);
  // #endregion resize-borrowed
}
