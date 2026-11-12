// mln_camera_for_tile(), the camera that shows one tile on a tile map.

#include <math.h>

#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"

typedef struct tile_case {
  mln_canonical_tile_id tile;
  uint32_t side;
  double scale_factor;
} tile_case;

// Tile 0/0/0, a mid-zoom tile, and two tiles at the edges of the world: the
// last column at the north edge, and the last tile of zoom 22 at the
// southeast corner. The mid-zoom tile also renders on a 256-pixel map at a
// scale factor of 2, which leaves the camera unchanged.
static const tile_case tile_cases[] = {
  {.tile = {0, 0, 0}, .side = 512, .scale_factor = 1.0},
  {.tile = {11, 1205, 1539}, .side = 512, .scale_factor = 1.0},
  {.tile = {11, 1205, 1539}, .side = 256, .scale_factor = 2.0},
  {.tile = {6, 63, 0}, .side = 256, .scale_factor = 1.0},
  {.tile = {22, 4194303, 4194303}, .side = 512, .scale_factor = 1.0},
};

#define TILE_CASE_COUNT (sizeof(tile_cases) / sizeof(tile_cases[0]))

// The center of a tile in Web Mercator, from the slippy map tile formula.
static mln_lat_lng tile_center(mln_canonical_tile_id tile) {
  const double tiles = ldexp(1.0, (int)tile.z);
  const double pi = acos(-1.0);
  const double y = pi * (1.0 - 2.0 * (tile.y + 0.5) / tiles);
  return (mln_lat_lng){
    .latitude = atan(sinh(y)) * 180.0 / pi,
    .longitude = (tile.x + 0.5) / tiles * 360.0 - 180.0,
  };
}

static mln_camera_options camera_for(
  mln_canonical_tile_id tile, uint32_t side
) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_OK(mln_camera_for_tile(
    tile, (mln_logical_size){.width = side, .height = side}, &camera, NULL
  ));
  return camera;
}

static void the_camera_centers_the_tile_at_the_zoom_that_fits_it(void) {
  for (size_t index = 0; index < TILE_CASE_COUNT; index += 1) {
    const tile_case* item = &tile_cases[index];
    const mln_camera_options camera = camera_for(item->tile, item->side);

    // Every field that moves the view, so the camera replaces a map's
    // earlier one.
    TEST_ASSERT_EQUAL_HEX32(
      MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
        MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
        MLN_CAMERA_OPTION_ROLL | MLN_CAMERA_OPTION_PADDING |
        MLN_CAMERA_OPTION_CENTER_ALTITUDE,
      camera.fields
    );
    const mln_lat_lng center = tile_center(item->tile);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, center.latitude, camera.center.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, center.longitude, camera.center.longitude);
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-12, item->tile.z - (item->side == 256 ? 1.0 : 0.0) + 1e-4, camera.zoom
    );
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.bearing);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.pitch);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.roll);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.center_altitude);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.padding.top);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.padding.left);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.padding.bottom);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, camera.padding.right);
  }

  // The zoom follows any side up to 512, not only powers of two.
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-12, 3.0 + log2(300.0 / 512.0) + 1e-4,
    camera_for((mln_canonical_tile_id){3, 1, 2}, 300).zoom
  );
}

static void the_camera_rejects_a_tile_the_map_cannot_fill(void) {
  const mln_canonical_tile_id tile = {.z = 2, .x = 1, .y = 3};
  const mln_logical_size side_512 = {.width = 512, .height = 512};
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.zoom = -1.0;

  MLN_TEST_INVALID(mln_camera_for_tile(
    tile, (mln_logical_size){.width = 512, .height = 256}, &camera,
    MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "square"));
  MLN_TEST_INVALID(
    mln_camera_for_tile(tile, (mln_logical_size){0}, &camera, NULL)
  );
  // A vector tile is 512 pixels wide, so a wider map shows several.
  MLN_TEST_INVALID(mln_camera_for_tile(
    tile, (mln_logical_size){.width = 1024, .height = 1024}, &camera, NULL
  ));
  MLN_TEST_INVALID(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 2, .x = 4, .y = 3}, side_512, &camera, NULL
  ));
  MLN_TEST_INVALID(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 2, .x = 1, .y = 4}, side_512, &camera, NULL
  ));
  // Below zoom 0: a 256-pixel map can't show all of zoom 0.
  MLN_TEST_INVALID(mln_camera_for_tile(
    (mln_canonical_tile_id){0}, (mln_logical_size){.width = 256, .height = 256},
    &camera, NULL
  ));
  // Past zoom 25.5.
  MLN_TEST_INVALID(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 26}, side_512, &camera, NULL
  ));
  MLN_TEST_INVALID(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = UINT32_MAX}, side_512, &camera, NULL
  ));
  TEST_ASSERT_EQUAL_DOUBLE(-1.0, camera.zoom);
  MLN_TEST_INVALID(mln_camera_for_tile(tile, side_512, NULL, NULL));
  mln_camera_options undersized = {.size = sizeof(mln_camera_options) - 1};
  MLN_TEST_INVALID(mln_camera_for_tile(tile, side_512, &undersized, NULL));

  // The edges of the accepted ranges.
  MLN_TEST_OK(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 25, .x = 33554431, .y = 33554431}, side_512,
    &camera, NULL
  ));
  MLN_TEST_OK(mln_camera_for_tile(
    (mln_canonical_tile_id){.z = 9},
    (mln_logical_size){.width = 1, .height = 1}, &camera, NULL
  ));
}

// A vector source's tiles are 512 pixels wide at their zoom, so a 512-pixel
// map shows tile z/x/y from the source's tile z/x/y, and a 256-pixel map from
// its parent.
static mln_canonical_tile_id source_tile_for(const tile_case* item) {
  const uint32_t shift = item->side == 256 ? 1U : 0U;
  return (mln_canonical_tile_id){
    .z = item->tile.z - shift,
    .x = item->tile.x >> shift,
    .y = item->tile.y >> shift,
  };
}

// The provider reads these until the harness reclaims a failed case's
// runtime, so they outlive the case.
static char tile_url[64];
static mln_test_provided_resource tile_resource[1];

// A static map loads every tile of a source that its view covers, where a
// tile map keeps the first and logs a warning. So the one request that a
// static map makes shows that a tile map renders exactly that tile. The
// source's minimum zoom is the tile's, so the map asks for no parent tile while
// the tile loads.
static void render_the_tile_and_expect_one_request(const tile_case* item) {
  const mln_canonical_tile_id source_tile = source_tile_for(item);
  (void)snprintf(
    tile_url, sizeof(tile_url), "custom://tiles/%u/%u/%u.pbf", source_tile.z,
    source_tile.x, source_tile.y
  );
  char style_json[512] = {0};
  (void)snprintf(
    style_json, sizeof(style_json),
    "{\"version\":8,\"sources\":{\"tiles\":{\"type\":\"vector\",\"tiles\":"
    "[\"custom://tiles/{z}/{x}/{y}.pbf\"],\"minzoom\":%u,\"maxzoom\":22}},"
    "\"layers\":[{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"tiles\","
    "\"source-layer\":\"any\"}]}",
    source_tile.z
  );
  tile_resource[0] = (mln_test_provided_resource){
    .url = tile_url, .response = {.status = MLN_RESOURCE_RESPONSE_STATUS_OK}
  };
  mln_test_provider* provider = mln_test_provider_create(tile_resource, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);

  mln_map_options options = mln_test_map_options();
  options.initial_extent = (mln_logical_extent){
    .width = item->side,
    .height = item->side,
    .scale_factor = item->scale_factor,
  };
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      mln_test_session_extent(
        fixture.session,
        (mln_logical_size){.width = item->side, .height = item->side}
      ),
      &completion.descriptor, NULL
    )
  );

  mln_test_load_style_and_wait(
    map, (mln_buffer_view){.data = style_json, .size = strlen(style_json)}
  );
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = camera_for(item->tile, item->side),
  };
  // The camera's zoom passes the map's default maximum for the tile at zoom 22.
  const mln_bound_options bounds = {
    .size = sizeof(mln_bound_options),
    .fields = MLN_BOUND_OPTION_MAX_ZOOM,
    .max_zoom = 25.5,
  };
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  TEST_ASSERT_EQUAL_INT_MESSAGE(
    1, atomic_load(&provider->request_count), tile_url
  );
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    1, mln_test_provider_requests(provider, tile_url), tile_url
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

static void a_map_at_the_tile_camera_loads_only_that_tile(void) {
  for (size_t index = 0; index < TILE_CASE_COUNT; index += 1) {
    render_the_tile_and_expect_one_request(&tile_cases[index]);
  }
}

MLN_TEST_GROUP {
  RUN_TEST(the_camera_centers_the_tile_at_the_zoom_that_fits_it);
  RUN_TEST(the_camera_rejects_a_tile_the_map_cannot_fill);
  RUN_TEST(a_map_at_the_tile_camera_loads_only_that_tile);
}
