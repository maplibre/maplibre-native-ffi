// Snippets for the docs site's Create a map page. They compile with the native
// tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
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

// Attaches a render target, as the Rendering guides show.
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
  mln_runtime_options runtime_options = {.size = sizeof(mln_runtime_options)};
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_create(&runtime_options, &runtime, NULL);
  // #endregion runtime

  // #region map
  mln_map_options map_options = {
    .size = sizeof(mln_map_options),
    .initial_extent = {.width = 800, .height = 600, .scale_factor = 2.0},
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
  };

  mln_map map = MLN_HANDLE_NULL;
  mln_runtime_create_map(runtime, &map_options, &map, &done, NULL);
  // #endregion map
  wait_for_done();

  mln_render_session session = attach_target(map);

  // #region close
  mln_render_session_dispose(session, NULL, NULL);
  mln_map_release(map, &done, NULL);
  // #endregion close
  wait_for_done();
  // #region close
  mln_runtime_release(runtime, &done, NULL);
  // #endregion close
  wait_for_done();
}
