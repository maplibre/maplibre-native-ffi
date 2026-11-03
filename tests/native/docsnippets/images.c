// Snippets for the docs site's Render images page. They compile with
// the native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

static atomic_bool finished;

static void on_done(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
  atomic_store(&finished, true);
}

static void wait_for_done(void) {
  while (!atomic_exchange(&finished, false)) {
  }
}

static void copy_pixels(mln_buffer_view data, uint32_t stride) {
  (void)data;
  (void)stride;
}

// #region read
static void on_pixels(
  void* user_data, const mln_completion_result* result
) {
  if (result->status == MLN_STATUS_OK) {
    const mln_texture_readback_result* readback = result->value;
    // readback->data is valid only during this callback.
    copy_pixels(readback->data, readback->info.stride);
  }
  on_done(user_data, result);
}
// #endregion read

void mln_doc_images(mln_runtime runtime);

void mln_doc_images(mln_runtime runtime) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region map
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent =
      {.width = 800, .height = 600, .scale_factor = 2.0},
    .map_mode = MLN_MAP_MODE_STATIC,
  };

  mln_map map = MLN_HANDLE_NULL;
  mln_runtime_create_map(runtime, &map_options, &map, &done, NULL);
  // #endregion map
  wait_for_done();

  // #region style
  mln_map_set_style_url(
    map,
    mln_buffer_view_from_cstr(
      "https://demotiles.maplibre.org/style.json"
    ),
    NULL, NULL
  );
  // #endregion style

  // #region attach
  mln_offscreen_descriptor offscreen = {
    .size = sizeof(mln_offscreen_descriptor),
    .logical_size = {.width = 800, .height = 600},
  };
  mln_render_session_attach_options attach_options = {
    .size = sizeof(mln_render_session_attach_options),
    // The library renders on its own thread.
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
  };

  // With no device, the session creates its own.
  mln_render_session session = MLN_HANDLE_NULL;
  mln_map_attach_offscreen(
    map, &offscreen, &attach_options, MLN_HANDLE_NULL, &session,
    &done, NULL
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
      .center = {.latitude = 48.8566, .longitude = 2.3522},
      .zoom = 4,
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

  mln_render_session_dispose(session, NULL, NULL);
  mln_map_release(map, &done, NULL);
  wait_for_done();
}

void mln_doc_images_tile(
  mln_runtime runtime, uint32_t z, uint32_t x, uint32_t y
);

void mln_doc_images_tile(
  mln_runtime runtime, uint32_t z, uint32_t x, uint32_t y
) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region tile
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent =
      {.width = 512, .height = 512, .scale_factor = 1.0},
    .map_mode = MLN_MAP_MODE_TILE,
  };
  mln_map map = MLN_HANDLE_NULL;
  mln_runtime_create_map(runtime, &map_options, &map, &done, NULL);
  // #endregion tile
  wait_for_done();

  // #region tile
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {.size = sizeof(mln_camera_options)},
  };
  mln_camera_for_tile(
    (mln_canonical_tile_id){.z = z, .x = x, .y = y},
    (mln_logical_size){.width = 512, .height = 512}, &update.camera,
    NULL
  );
  mln_map_update_camera(map, &update, NULL, NULL);
  // #endregion tile

  mln_map_release(map, &done, NULL);
  wait_for_done();
}
