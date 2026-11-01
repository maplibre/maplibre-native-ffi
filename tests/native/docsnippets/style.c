// Snippets for the docs site's Style page. They compile with the native tests,
// which never run them.

#include <maplibre_native_c.h>
#include <stddef.h>
#include <stdint.h>

static mln_map map = MLN_HANDLE_NULL;

static mln_premultiplied_rgba8_image draw_image(mln_buffer_view image_id) {
  (void)image_id;
  static const uint8_t pixel[4] = {255, 0, 0, 255};
  const mln_premultiplied_rgba8_image image = {
    .size = sizeof(mln_premultiplied_rgba8_image),
    .width = 1,
    .height = 1,
    .stride = 4,
    .pixels = pixel,
    .byte_length = sizeof(pixel),
  };
  return image;
}

void mln_doc_style(const uint8_t* pixels);

// pixels holds 16 x 16 premultiplied RGBA pixels.
void mln_doc_style(const uint8_t* pixels) {
  // #region load
  mln_map_set_style_url(
    map, mln_buffer_view_from_cstr("https://demotiles.maplibre.org/style.json"),
    NULL, NULL
  );
  // #endregion load

  // #region add
  mln_map_add_style_source_json(
    map, mln_buffer_view_from_cstr("satellite"),
    mln_buffer_view_from_cstr(
      "{\"type\":\"raster\",\"tileSize\":256,"
      "\"tiles\":[\"https://tiles.example.com/{z}/{x}/{y}.png\"]}"
    ),
    NULL, NULL
  );

  // Draw the layer under the "countries-label" layer.
  mln_map_add_style_layer_json(
    map,
    mln_buffer_view_from_cstr(
      "{\"id\":\"satellite\",\"type\":\"raster\",\"source\":\"satellite\"}"
    ),
    mln_buffer_view_from_cstr("countries-label"), NULL, NULL
  );
  // #endregion add

  // #region geojson
  mln_geo_json_source_data data = MLN_HANDLE_NULL;
  mln_geo_json_source_data_create(
    mln_buffer_view_from_cstr(
      "{\"type\":\"Point\",\"coordinates\":[-122.4,37.8]}"
    ),
    NULL, &data, NULL
  );
  mln_map_add_geo_json_source_data(
    map, mln_buffer_view_from_cstr("points"), data, NULL, NULL
  );
  // The source keeps its own reference to the data.
  mln_geo_json_source_data_release(data);

  // Later, prepare new data with the same options, and set it.
  mln_geo_json_source_data next = MLN_HANDLE_NULL;
  mln_geo_json_source_data_create(
    mln_buffer_view_from_cstr(
      "{\"type\":\"Point\",\"coordinates\":[2.35,48.86]}"
    ),
    NULL, &next, NULL
  );
  mln_map_set_geo_json_source_data(
    map, mln_buffer_view_from_cstr("points"), next, NULL, NULL
  );
  mln_geo_json_source_data_release(next);
  // #endregion geojson

  // #region set-property
  const mln_buffer_view satellite = mln_buffer_view_from_cstr("satellite");
  const mln_buffer_view boundaries =
    mln_buffer_view_from_cstr("countries-boundary");

  // Show both changes in the same frame.
  mln_map_begin_command_group(map, NULL, NULL);
  mln_map_set_style_layer_property(
    map, satellite, mln_buffer_view_from_cstr("visibility"),
    mln_buffer_view_from_cstr("\"none\""), NULL, NULL
  );
  mln_map_set_style_layer_property(
    map, boundaries, mln_buffer_view_from_cstr("line-color"),
    mln_buffer_view_from_cstr(
      "[\"interpolate\",[\"linear\"],[\"zoom\"],5,\"#9cf\",12,\"#06c\"]"
    ),
    NULL, NULL
  );
  mln_map_end_command_group(map, NULL, NULL);
  // #endregion set-property

  // #region image
  const mln_premultiplied_rgba8_image image = {
    .size = sizeof(mln_premultiplied_rgba8_image),
    .width = 16,
    .height = 16,
    .stride = 16 * 4,
    .pixels = pixels,
    .byte_length = 16 * 16 * 4,
  };
  mln_map_set_style_image(
    map, mln_buffer_view_from_cstr("marker"), &image, NULL, NULL, NULL
  );

  mln_map_add_style_layer_json(
    map,
    mln_buffer_view_from_cstr(
      "{\"id\":\"markers\",\"type\":\"symbol\",\"source\":\"points\","
      "\"layout\":{\"icon-image\":\"marker\"}}"
    ),
    (mln_buffer_view){0}, NULL, NULL
  );
  // #endregion image
}

// #region missing-image
static void on_events(void* user_data, const mln_event_batch_view* batch) {
  (void)user_data;
  for (size_t i = 0; i < batch->event_count; i++) {
    const mln_runtime_event* event =
      MLN_STRIDED_AT(mln_runtime_event, batch->events, batch->event_size, i);
    if (
      event->source != map ||
      event->type != MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING
    ) {
      continue;
    }
    // The message holds the image's ID.
    const mln_premultiplied_rgba8_image image = draw_image(event->message);
    mln_map_set_style_image(map, event->message, &image, NULL, NULL, NULL);
  }
}
// #endregion missing-image

void mln_doc_style_events(mln_runtime runtime);

void mln_doc_style_events(mln_runtime runtime) {
  const mln_runtime_event_handler handler = {
    .size = sizeof(mln_runtime_event_handler),
    .callback = on_events,
  };
  mln_runtime_set_event_handler(runtime, &handler, NULL, NULL);
}
