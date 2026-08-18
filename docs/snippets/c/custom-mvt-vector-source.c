// A custom MVT vector source that the host fills from its own tile store.

#include <maplibre_native_c.h>
#include <stdint.h>
#include <string.h>

typedef struct tile_store tile_store;

// The host supplies a concurrent lookup. Its bytes remain valid until return.
bool tile_store_find(
  const tile_store* store, uint8_t z, uint32_t x, uint32_t y,
  const uint8_t** out_bytes, size_t* out_size
);

typedef struct host_tiles {
  mln_map map;
  const tile_store* store;
} host_tiles;

static mln_buffer_view view(const char* text) {
  return (mln_buffer_view){.data = text, .size = strlen(text)};
}

void queue_tile_bytes(
  const host_tiles* host, mln_canonical_tile_id tile_id, const uint8_t* bytes,
  size_t size
);
void queue_tile_error(
  const host_tiles* host, mln_canonical_tile_id tile_id, const char* message
);

// #region fetch
static void fetch_tile(void* user_data, mln_canonical_tile_id tile_id) {
  // This callback can run on a worker thread. Queue the result to the map
  // owner thread before calling set_tile_data or set_tile_error.
  const host_tiles* host = user_data;
  const uint8_t* bytes = NULL;
  size_t size = 0;
  if (!tile_store_find(
        host->store, (uint8_t)tile_id.z, tile_id.x, tile_id.y, &bytes, &size
      )) {
    queue_tile_error(host, tile_id, "tile missing");
    return;
  }
  queue_tile_bytes(host, tile_id, bytes, size);
}
// #endregion fetch

mln_status deliver_tile_bytes(
  mln_map map, mln_canonical_tile_id tile_id, const uint8_t* bytes, size_t size
) {
  // #region deliver
  // A zero-length view is an empty tile. The call copies accepted bytes.
  const mln_buffer_view data = {.data = bytes, .size = size};
  return mln_map_set_custom_mvt_vector_source_tile_data(
    map, view("host-tiles"), tile_id, data
  );
  // #endregion deliver
}

mln_status deliver_tile_error(
  mln_map map, mln_canonical_tile_id tile_id, const char* message
) {
  return mln_map_set_custom_mvt_vector_source_tile_error(
    map, view("host-tiles"), tile_id, view(message)
  );
}

mln_status show_host_tiles(mln_map map, host_tiles* host) {
  // #region source
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fetch_tile = fetch_tile;
  options.user_data = host;
  mln_status status =
    mln_map_add_custom_mvt_vector_source(map, view("host-tiles"), &options);
  if (status != MLN_STATUS_OK) return status;
  // #endregion source

  // #region layer
  const char layer[] =
    "{\"id\":\"buildings\",\"type\":\"fill\",\"source\":\"host-tiles\","
    "\"source-layer\":\"building\"}";
  // #endregion layer

  return mln_map_add_style_layer_json(map, view(layer), view(""));
}

mln_status refetch_one_tile(mln_map map, mln_canonical_tile_id tile_id) {
  // #region invalidate
  return mln_map_invalidate_custom_mvt_vector_source_tile(
    map, view("host-tiles"), tile_id
  );
  // #endregion invalidate
}
