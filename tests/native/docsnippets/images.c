// Snippets for the docs site's Render images page. They compile with the
// native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

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

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
static mln_lat_lng tile_center(uint32_t z, uint32_t x, uint32_t y) {
  (void)z;
  (void)x;
  (void)y;
  return (mln_lat_lng){0};
}

static void copy_pixels(mln_buffer_view data, uint32_t stride) {
  (void)data;
  (void)stride;
}

// #region read
static void on_pixels(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const mln_texture_readback_result* readback = result->value;
    // readback->data is valid only during this callback.
    copy_pixels(readback->data, readback->info.stride);
  }
  on_done(user_data, result);
}
// #endregion read

void mln_doc_images(mln_runtime runtime, uint32_t z, uint32_t x, uint32_t y);

void mln_doc_images(mln_runtime runtime, uint32_t z, uint32_t x, uint32_t y) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region map
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent = {.width = 512, .height = 512, .scale_factor = 1.0},
    .map_mode = MLN_MAP_MODE_TILE,
  };

  mln_map map = MLN_HANDLE_NULL;
  mln_runtime_create_map(runtime, &map_options, &map, &done, NULL);
  // #endregion map
  wait_for_done();

  // #region style
  mln_map_set_style_url(
    map, mln_buffer_view_from_cstr("https://demotiles.maplibre.org/style.json"),
    NULL, NULL
  );
  // #endregion style

  // #region attach
  // NULL options select the default adapter.
  mln_graphics_device device = MLN_HANDLE_NULL;
  mln_graphics_device_create(NULL, &device, NULL);

  mln_graphics_device_texture_descriptor texture = {
    .size = sizeof(mln_graphics_device_texture_descriptor),
    .extent = map_options.initial_extent,
  };
  mln_render_session_attach_options attach_options = {
    .size = sizeof(mln_render_session_attach_options),
    // The library renders on its own thread.
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
  };

  mln_render_session session = MLN_HANDLE_NULL;
  mln_map_attach_graphics_device_texture(
    map, device, &texture, &attach_options, &session, &done, NULL
  );
  // #endregion attach
  wait_for_done();

  // #region image
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM,
      .center = tile_center(z, x, y),
      .zoom = z,
    },
  };
  mln_map_update_camera(map, &update, NULL, NULL);

  mln_map_request_still_image(map, &done, NULL);
  // #endregion image
  wait_for_done();

  // #region read
  const mln_completion read = {
    .size = sizeof(mln_completion), .callback = on_pixels
  };

  mln_render_session_read_texture(session, &read, NULL);
  // #endregion read
  wait_for_done();

  // #region close
  mln_render_session_dispose(session, NULL, NULL);
  mln_graphics_device_release(device);
  mln_map_release(map, &done, NULL);
  // #endregion close
  wait_for_done();
}
