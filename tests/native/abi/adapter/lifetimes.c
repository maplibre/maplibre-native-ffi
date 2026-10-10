// Adapter owners that end native lifetimes for a host: arenas and owner
// tokens.

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/test_support.h"

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
  MLN_TEST_OK(
    mln_adapter_arena_adopt_release(arena, read_arena_context, context, NULL)
  );
  MLN_TEST_OK(mln_adapter_arena_adopt_handle(arena, map, NULL));
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

  MLN_TEST_INVALID(mln_adapter_arena_adopt_release(
    NULL, count_release, &probe, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "arena must not be null"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_UINT(1, probe.calls);
  MLN_TEST_INVALID(
    mln_adapter_arena_adopt_release(NULL, NULL, &probe, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "release must not be null"),
    mln_test_last_error()
  );
  MLN_TEST_INVALID(
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

MLN_TEST_GROUP {
  RUN_TEST(destroying_an_arena_releases_what_it_adopted);
  RUN_TEST(a_failed_arena_adoption_releases_at_once);
  RUN_TEST(an_owner_token_disposes_its_owner_only_when_finalized);
}
