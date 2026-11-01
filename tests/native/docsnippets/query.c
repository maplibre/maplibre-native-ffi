// Snippets for the docs site's Query page. They compile with the native
// tests, which never run them.

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

static void copy_feature(mln_buffer_view json) { (void)json; }

// #region rendered
static void on_rendered(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const uint8_t* values = result->value;
    for (size_t i = 0; i < result->value_count; i++) {
      // Step through the results by value_size, not by sizeof.
      const mln_rendered_feature* hit =
        (const void*)(values + i * result->value_size);
      // The views are valid only during this callback.
      copy_feature(hit->feature);
    }
  }
  on_done(user_data, result);
}
// #endregion rendered

// #region source
static void on_source(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const uint8_t* values = result->value;
    for (size_t i = 0; i < result->value_count; i++) {
      const mln_source_feature* hit =
        (const void*)(values + i * result->value_size);
      copy_feature(hit->feature);
    }
  }
  on_done(user_data, result);
}
// #endregion source

void mln_doc_query(mln_render_session session, mln_screen_point point);

void mln_doc_query(mln_render_session session, mln_screen_point point) {
  // #region rendered
  const mln_rendered_query_geometry box = {
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
    .data.box = {
      .min = {.x = point.x - 5, .y = point.y - 5},
      .max = {.x = point.x + 5, .y = point.y + 5},
    },
  };
  const mln_buffer_view layers[] = {mln_buffer_view_from_cstr("poi")};
  const mln_rendered_feature_query_options options = {
    .size = sizeof(mln_rendered_feature_query_options),
    .fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS,
    .layer_ids = layers,
    .layer_id_count = 1,
  };
  const mln_completion rendered = {
    .size = sizeof(mln_completion), .callback = on_rendered
  };
  mln_render_session_query_rendered_features(
    session, &box, &options, &rendered, NULL
  );
  // #endregion rendered
  wait_for_done();

  // #region source
  const mln_buffer_view source_layers[] = {
    mln_buffer_view_from_cstr("transportation")
  };
  const mln_source_feature_query_options source_options = {
    .size = sizeof(mln_source_feature_query_options),
    .fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS,
    .source_layer_ids = source_layers,
    .source_layer_id_count = 1,
    // A filter from the MapLibre style spec, as JSON.
    .filter =
      mln_buffer_view_from_cstr("[\"==\", [\"get\", \"class\"], \"motorway\"]"),
  };
  const mln_completion source = {
    .size = sizeof(mln_completion), .callback = on_source
  };
  mln_render_session_query_source_features(
    session, mln_buffer_view_from_cstr("openmaptiles"), &source_options,
    &source, NULL
  );
  // #endregion source
  wait_for_done();
}
