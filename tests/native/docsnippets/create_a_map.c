// Snippets for the docs site's Create a map page. They compile with
// the native tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stddef.h>

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

static void report_failure(mln_buffer_view message) { (void)message; }

// #region wait
static void on_camera_moved(void*, const mln_completion_result*) {
  // The camera has moved.
}
// #endregion wait

// #region errors
static void on_camera_result(
  void*, const mln_completion_result* result
) {
  if (result->status != MLN_STATUS_OK) {
    report_failure(result->diagnostic);
  }
}
// #endregion errors

// Attaches a render target, as the render target guides show.
static mln_render_session attach_target(mln_map map) {
  (void)map;
  return MLN_HANDLE_NULL;
}

void mln_doc_create_a_map(void);

void mln_doc_create_a_map(void) {
  const mln_completion done = {
    .size = sizeof(mln_completion), .callback = on_done
  };

  // #region runtime
  mln_runtime_options runtime_options = {
    .size = sizeof(mln_runtime_options)
  };
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_create(&runtime_options, &runtime, NULL);
  // #endregion runtime

  // #region map
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent =
      {.width = 800, .height = 600, .scale_factor = 2.0},
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
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

  const mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_ZOOM,
      .zoom = 3,
    },
  };

  // #region wait
  const mln_completion moved = {
    .size = sizeof(mln_completion), .callback = on_camera_moved
  };
  mln_map_update_camera(map, &update, &moved, NULL);

  // Pass no completion to ignore the result.
  mln_map_update_camera(map, &update, NULL, NULL);
  // #endregion wait

  // #region errors
  const mln_completion result = {
    .size = sizeof(mln_completion), .callback = on_camera_result
  };
  mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};
  const mln_status status =
    mln_map_update_camera(map, &update, &result, &diagnostic);
  if (status != MLN_STATUS_OK) {
    report_failure(mln_buffer_view_from_cstr(diagnostic.message));
  }
  // #endregion errors

  mln_render_session session = attach_target(map);

  // #region close
  mln_render_session_dispose(session, NULL, NULL);
  mln_map_dispose(map, NULL);
  mln_runtime_release(runtime, &done, NULL);
  // #endregion close
  wait_for_done();
}
