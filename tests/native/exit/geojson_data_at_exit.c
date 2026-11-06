// A process that exits while a GeoJSON source prepares its data on MapLibre's
// background threads, with newer data waiting behind it.
// See probe.h.

#include "probe.h"

int main(void) {
  probe_start();
  const mln_map map = probe_create_map(probe_create_runtime());
  PROBE_AWAIT(
    "loading a style",
    mln_map_set_style_json(
      map,
      mln_buffer_view_from_cstr("{\"version\":8,\"sources\":{},\"layers\":[]}"),
      &completion, NULL
    )
  );
  PROBE_AWAIT(
    "adding a GeoJSON source", mln_map_add_geo_json_source(
                                 map, mln_buffer_view_from_cstr("points"), NULL,
                                 (mln_buffer_view){0}, &completion, NULL
                               )
  );
  for (int index = 0; index < 10; index += 1) {
    probe_require(
      mln_map_set_geo_json_source_data(
        map, mln_buffer_view_from_cstr("points"),
        mln_buffer_view_from_cstr(probe_points_json),
        &probe_discarding_completion, NULL
      ),
      "setting GeoJSON data"
    );
  }
  return 0;
}
