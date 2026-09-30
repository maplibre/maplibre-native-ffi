// Adapter owners that end native lifetimes for a host: arenas, owner tokens,
// and the retirement call that custom source callbacks receive.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

// What an adopted release saw when it ran.
typedef struct release_probe {
  unsigned calls;
  int observed;
} release_probe;

static release_probe arena_release;

// Reads the arena allocation that is its context, which stays valid while the
// arena runs its releases.
static void read_arena_context(void* context) {
  arena_release.calls += 1;
  arena_release.observed = *(const int*)context;
}

static void count_release(void* context) {
  ((release_probe*)context)->calls += 1;
}

// Destroying an arena runs each adopted release once, before it frees the
// allocations that the release may read, and disposes each adopted handle.
static void destroying_an_arena_releases_what_it_adopted(void) {
  arena_release = (release_probe){0};
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_adapter_create_map(runtime);
  void* arena = mln_adapter_arena_create();
  TEST_ASSERT_NOT_NULL(arena);

  int* context = mln_adapter_arena_allocate(arena, sizeof(int), _Alignof(int));
  TEST_ASSERT_NOT_NULL(context);
  TEST_ASSERT_EQUAL_INT(0, *context);
  *context = 42;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_adapter_arena_adopt_release(arena, read_arena_context, context, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_arena_adopt_handle(arena, map, NULL)
  );
  TEST_ASSERT_EQUAL_UINT(0, arena_release.calls);
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(map));

  mln_adapter_arena_destroy(arena);
  TEST_ASSERT_EQUAL_UINT(1, arena_release.calls);
  TEST_ASSERT_EQUAL_INT(42, arena_release.observed);
  TEST_ASSERT_FALSE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_runtime(runtime);
}

// Ownership transfers on entry, so an adoption that fails releases at once.
static void a_failed_arena_adoption_releases_at_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_adapter_create_map(runtime);
  release_probe probe = {0};

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_arena_adopt_release(
      NULL, count_release, &probe, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "arena must not be null"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_UINT(1, probe.calls);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_arena_adopt_release(NULL, NULL, &probe, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "release must not be null"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_arena_adopt_handle(NULL, map, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "arena must not be null"),
    mln_test_last_error()
  );
  TEST_ASSERT_FALSE(mln_test_adapter_map_is_live(map));
  mln_test_destroy_runtime(runtime);
}

// Destroying a token after the host released its owner explicitly leaves the
// owner alone. Finalizing a token disposes the owner.
static void an_owner_token_disposes_its_owner_only_when_finalized(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map released = mln_test_adapter_create_map(runtime);
  mln_map finalized = mln_test_adapter_create_map(runtime);

  void* token = mln_adapter_owner_token_create(released);
  TEST_ASSERT_NOT_NULL(token);
  mln_adapter_owner_token_destroy(token);
  TEST_ASSERT_TRUE(mln_test_adapter_map_is_live(released));
  mln_test_destroy_map(released);

  token = mln_adapter_owner_token_create(finalized);
  TEST_ASSERT_NOT_NULL(token);
  mln_adapter_owner_finalize(token);
  TEST_ASSERT_FALSE(mln_test_adapter_map_is_live(finalized));
  mln_test_destroy_runtime(runtime);
}

// Records each tile callback that retirement invokes.
typedef struct retire_probe {
  size_t count;
  struct {
    bool fetch;
    void* user_data;
    mln_canonical_tile_id tile;
  } calls[4];
} retire_probe;

static retire_probe retired;

static void record_tile(
  bool fetch, void* user_data, mln_canonical_tile_id tile
) {
  TEST_ASSERT_LESS_THAN_size_t(4, retired.count);
  retired.calls[retired.count].fetch = fetch;
  retired.calls[retired.count].user_data = user_data;
  retired.calls[retired.count].tile = tile;
  retired.count += 1;
}

static void record_fetch(void* user_data, mln_canonical_tile_id tile) {
  record_tile(true, user_data, tile);
}

static void record_cancel(void* user_data, mln_canonical_tile_id tile) {
  record_tile(false, user_data, tile);
}

typedef void (*retire_function)(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
);

static void retire_mvt_vector(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
) {
  mln_adapter_custom_mvt_vector_callbacks_retire(
    fetch_tile, cancel_tile, user_data
  );
}

// Checks one retirement call: the callbacks that were not null each ran once,
// fetch before cancel, with the context and the retirement tile id.
static void assert_retired(
  const char* label, size_t expected, bool first_is_fetch, void* user_data
) {
  TEST_ASSERT_EQUAL_size_t_MESSAGE(expected, retired.count, label);
  for (size_t index = 0; index < retired.count; ++index) {
    TEST_ASSERT_EQUAL_MESSAGE(
      index == 0 && first_is_fetch, retired.calls[index].fetch, label
    );
    TEST_ASSERT_EQUAL_PTR_MESSAGE(
      user_data, retired.calls[index].user_data, label
    );
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      UINT8_MAX, retired.calls[index].tile.z, label
    );
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, retired.calls[index].tile.x, label);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, retired.calls[index].tile.y, label);
  }
}

static void retiring_custom_source_callbacks_calls_each_once(void) {
  static const struct {
    const char* label;
    retire_function retire;
  } sources[] = {
    {"custom geometry", mln_adapter_custom_geometry_callbacks_retire},
    {"custom MVT vector", retire_mvt_vector},
  };
  int context = 0;
  for (size_t index = 0; index < sizeof(sources) / sizeof(sources[0]);
       ++index) {
    retired = (retire_probe){0};
    sources[index].retire(record_fetch, record_cancel, &context);
    assert_retired(sources[index].label, 2, true, &context);

    retired = (retire_probe){0};
    sources[index].retire(NULL, record_cancel, &context);
    assert_retired(sources[index].label, 1, false, &context);

    retired = (retire_probe){0};
    sources[index].retire(NULL, NULL, &context);
    assert_retired(sources[index].label, 0, false, &context);
  }
}

MLN_TEST_GROUP {
  RUN_TEST(destroying_an_arena_releases_what_it_adopted);
  RUN_TEST(a_failed_arena_adoption_releases_at_once);
  RUN_TEST(an_owner_token_disposes_its_owner_only_when_finalized);
  RUN_TEST(retiring_custom_source_callbacks_calls_each_once);
}
