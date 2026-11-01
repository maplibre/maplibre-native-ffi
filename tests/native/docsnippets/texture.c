// Snippets for the docs site's Render to a texture page. They compile with the
// native tests, which never run them.

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
    mln_render_session_acquire_frame(session, &frame, NULL) != MLN_STATUS_OK
  ) {
    return;  // No new frame yet.
  }

  mln_vulkan_texture_frame texture = {.size = sizeof(mln_vulkan_texture_frame)};
  mln_acquired_frame_get_vulkan_texture(frame, &texture, NULL);

  // Sample the texture, and wait for those reads to finish.
  draw_map(texture.image_view);

  mln_acquired_frame_release(frame, NULL, NULL);
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
  // #region attach-owned
  const mln_vulkan_owned_texture_descriptor texture = {
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    // The texture's size in logical pixels, and its scale factor.
    .extent = {.width = 800, .height = 600, .scale_factor = 2.0},
    // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
    .context = vulkan,
  };
  const mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .frame_wake = {.callback = on_frame_wake},
    // Only if your renderer also submits to the graphics queue.
    .queue_lock = {.lock = lock_queue, .unlock = unlock_queue},
  };

  mln_map_attach_vulkan_owned_texture(
    map, &texture, &options, &session, &done, NULL
  );
  // #endregion attach-owned
  wait_for_done();

  draw_frame();
  mln_render_session_dispose(session, &done, NULL);
  session = MLN_HANDLE_NULL;
  wait_for_done();

  // #region attach-borrowed
  // Your VkImage and VkImageView for each texture of the ring.
  const mln_vulkan_borrowed_texture textures[2] = {
    {.image = images[0], .image_view = views[0]},
    {.image = images[1], .image_view = views[1]},
  };
  mln_vulkan_borrowed_texture_descriptor ring = {
    .size = sizeof(mln_vulkan_borrowed_texture_descriptor),
    .extent = {.width = 800, .height = 600, .scale_factor = 2.0},
    // The size of your textures in device pixels.
    .physical_width = 1600,
    .physical_height = 1200,
    .context = vulkan,
    .textures = textures,
    .texture_count = 2,
    .format = 37,       // VK_FORMAT_R8G8B8A8_UNORM
    .final_layout = 5,  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  };

  // The same attach options as for an owned ring.
  mln_map_attach_vulkan_borrowed_texture(
    map, &ring, &options, &session, &done, NULL
  );
  // #endregion attach-borrowed
  wait_for_done();

  // #region resize-borrowed
  // Your new textures, at the new size.
  const mln_vulkan_borrowed_texture new_textures[2] = {
    {.image = new_images[0], .image_view = new_views[0]},
    {.image = new_images[1], .image_view = new_views[1]},
  };
  ring.extent =
    (mln_logical_extent){.width = 1024, .height = 768, .scale_factor = 2.0};
  ring.physical_width = 2048;
  ring.physical_height = 1536;
  ring.textures = new_textures;

  mln_render_session_set_vulkan_borrowed_texture_target(
    session, &ring, &done, NULL
  );
  // #endregion resize-borrowed
  wait_for_done();

  // #region resize-borrowed
  destroy_textures(images, views);  // vkDestroyImageView and vkDestroyImage
  // #endregion resize-borrowed
}
