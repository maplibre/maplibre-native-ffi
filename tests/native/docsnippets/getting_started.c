// Snippets for the docs site's Getting started page. They compile with the
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

static void copy_pixels(mln_buffer_view data, uint32_t stride) {
  (void)data;
  (void)stride;
}

// #region render
static void on_pixels(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const mln_texture_readback_result* readback = result->value;
    // readback->data is valid only during this callback.
    copy_pixels(readback->data, readback->info.stride);
  }
  on_done(user_data, result);
}
// #endregion render

void mln_doc_getting_started(void);

void mln_doc_getting_started(void) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region runtime
  mln_runtime_options runtime_options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_create(&runtime_options, &runtime, NULL);
  // #endregion runtime

  // #region map
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent = {.width = 512, .height = 512, .scale_factor = 1.0},
    .map_mode = MLN_MAP_MODE_STATIC,
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

  // #region render
  mln_map_request_still_image(map, &done, NULL);
  // #endregion render
  wait_for_done();

  // #region render
  const mln_completion read = {
    .size = sizeof(mln_completion), .callback = on_pixels
  };

  mln_render_session_read_texture(session, &read, NULL);
  // #endregion render
  wait_for_done();

  // #region close
  mln_render_session_dispose(session, NULL, NULL);
  mln_graphics_device_release(device);
  mln_map_release(map, &done, NULL);
  mln_runtime_release(runtime, &done, NULL);
  // #endregion close
}
