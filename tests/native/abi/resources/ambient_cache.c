// The ambient cache: each maintenance operation, and a new budget, observed
// through what the next request for a cached resource tells the provider.

#include "support/resources.h"
#include "support/test_support.h"

static const char cached_style_url[] = "custom://ambient/style.json";
static const char cached_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static void ambient_cache_calls_validate_their_arguments(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_completion completion = mln_test_discard_completion();
  static const uint32_t unknown_operations[] = {0, 5, 999};
  for (size_t index = 0;
       index < sizeof(unknown_operations) / sizeof(unknown_operations[0]);
       index += 1) {
    MLN_TEST_INVALID(mln_runtime_run_ambient_cache_operation(
      runtime, unknown_operations[index], &completion, NULL
    ));
  }
  MLN_TEST_INVALID(mln_runtime_run_ambient_cache_operation(
    MLN_HANDLE_NULL, MLN_AMBIENT_CACHE_OPERATION_CLEAR, &completion, NULL
  ));
  MLN_TEST_INVALID(mln_runtime_run_ambient_cache_operation(
    runtime, MLN_AMBIENT_CACHE_OPERATION_CLEAR, NULL, NULL
  ));
  MLN_TEST_INVALID(
    mln_runtime_set_maximum_ambient_cache_size(runtime, 1024, NULL, NULL)
  );
  MLN_TEST_INVALID(mln_runtime_set_maximum_ambient_cache_size(
    MLN_HANDLE_NULL, 1024, &completion, NULL
  ));
  mln_test_destroy_runtime(runtime);
}

// What a cache change leaves of an entry the provider answered earlier.
typedef enum cached_entry {
  // Kept as it was: the next request revalidates with the stored metadata.
  ENTRY_KEPT,
  // Kept but marked for revalidation: the next request also carries its bytes,
  // because the map may not use them until the provider answers.
  ENTRY_INVALIDATED,
  // Gone: the next request carries nothing.
  ENTRY_REMOVED,
} cached_entry;

typedef enum cache_change {
  CHANGE_NONE,
  CHANGE_OPERATION,
  CHANGE_BUDGET,
} cache_change;

typedef struct cache_case {
  const char* label;
  cache_change change;
  uint32_t operation;
  uint64_t budget;
  cached_entry entry;
} cache_case;

static const cache_case cache_cases[] = {
  {"no change", CHANGE_NONE, 0, 0, ENTRY_KEPT},
  {"a pack", CHANGE_OPERATION, MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE, 0,
   ENTRY_KEPT},
  {"an invalidation", CHANGE_OPERATION, MLN_AMBIENT_CACHE_OPERATION_INVALIDATE,
   0, ENTRY_INVALIDATED},
  {"a clear", CHANGE_OPERATION, MLN_AMBIENT_CACHE_OPERATION_CLEAR, 0,
   ENTRY_REMOVED},
  {"a reset", CHANGE_OPERATION, MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE, 0,
   ENTRY_REMOVED},
  {"a generous budget", CHANGE_BUDGET, 0, 64 * 1024 * 1024, ENTRY_KEPT},
  {"a zero budget", CHANGE_BUDGET, 0, 0, ENTRY_REMOVED},
};

// Loads the cached style on a new map. The runtime's default database is in
// memory and lives only while something holds it, so a case keeps its first
// map until the next load has read the cache.
static mln_map load_style(mln_runtime runtime) {
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, cached_style_url));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  return map;
}

static void apply_change(mln_runtime runtime, const cache_case* row) {
  switch (row->change) {
    case CHANGE_NONE:
      return;
    case CHANGE_OPERATION: {
      mln_test_completion completion = mln_test_completion_default(0);
      MLN_TEST_OK_MESSAGE(
        mln_runtime_run_ambient_cache_operation(
          runtime, row->operation, &completion.descriptor, NULL
        ),
        row->label
      );
      MLN_TEST_OK_MESSAGE(mln_test_completion_finish(&completion), row->label);
      mln_test_completion_destroy(&completion);
      return;
    }
    case CHANGE_BUDGET: {
      mln_test_completion completion = mln_test_completion_default(0);
      MLN_TEST_OK_MESSAGE(
        mln_runtime_set_maximum_ambient_cache_size(
          runtime, row->budget, &completion.descriptor, NULL
        ),
        row->label
      );
      MLN_TEST_OK_MESSAGE(mln_test_completion_finish(&completion), row->label);
      mln_test_completion_destroy(&completion);
      return;
    }
  }
}

// Each maintenance operation and budget either keeps a cached entry, marks it
// for revalidation, or removes it, which the next request for it shows.
static void ambient_cache_changes_reach_the_next_request(void) {
  static const mln_test_provided_resource resources[] = {
    {.url = cached_style_url,
     .response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
       .bytes = (const uint8_t*)cached_style_json,
       .byte_count = sizeof(cached_style_json) - 1,
       .etag = "\"ambient\"",
       // Fresh until 2100, so only a cache change makes it stale.
       .has_expires = true,
       .expires_unix_ms = 4102444800000,
     }},
  };
  for (size_t index = 0; index < sizeof(cache_cases) / sizeof(cache_cases[0]);
       index += 1) {
    const cache_case* row = &cache_cases[index];
    mln_test_provider* provider = mln_test_provider_create(resources, 1);
    mln_runtime runtime = mln_test_create_runtime();
    mln_test_provider_install(runtime, provider);
    mln_map first = load_style(runtime);
    apply_change(runtime, row);
    mln_map second = load_style(runtime);
    // A kept entry loads the style from the cache before its revalidation
    // reaches the provider.
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_provider_wait_for_requests(provider, cached_style_url, 2),
      row->label
    );

    const mln_test_provider_request* next =
      mln_test_provider_request_at(provider, cached_style_url, 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(next, row->label);
    TEST_ASSERT_EQUAL_MESSAGE(
      row->entry != ENTRY_REMOVED, next->has_prior_etag, row->label
    );
    if (row->entry != ENTRY_REMOVED) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "\"ambient\"", next->prior_etag, row->label
      );
    }
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      row->entry == ENTRY_INVALIDATED ? sizeof(cached_style_json) - 1 : 0,
      next->prior_data_size, row->label
    );
    mln_test_destroy_map(second);
    mln_test_destroy_map(first);
    mln_test_destroy_runtime(runtime);
    mln_test_provider_destroy(provider);
  }
}

typedef struct shared_cache_probe {
  const char* cache_path;
  // How many threads have a runtime with an operation in flight. Each thread
  // creates its runtime after the threads before it, and finishes only once
  // every thread has one.
  atomic_int started;
} shared_cache_probe;

typedef struct shared_cache_thread {
  shared_cache_probe* probe;
  int index;
  mln_status create_status;
  mln_status operation_status;
  mln_status close_status;
} shared_cache_thread;

static void touch_shared_cache(void* argument) {
  shared_cache_thread* thread = argument;
  shared_cache_probe* probe = thread->probe;
  (void)mln_test_wait_for_count(&probe->started, thread->index);

  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = probe->cache_path;
  mln_runtime runtime = MLN_HANDLE_NULL;
  thread->create_status = mln_runtime_create(&options, &runtime, NULL);
  mln_test_completion operation = mln_test_completion_default(0);
  bool submitted = false;
  if (thread->create_status == MLN_STATUS_OK) {
    thread->operation_status = mln_runtime_run_ambient_cache_operation(
      runtime, MLN_AMBIENT_CACHE_OPERATION_INVALIDATE, &operation.descriptor,
      NULL
    );
    submitted = thread->operation_status == MLN_STATUS_OK;
  }
  if (!submitted) {
    mln_test_completion_reject(&operation);
    mln_test_completion_destroy(&operation);
  }
  atomic_fetch_add(&probe->started, 1);
  mln_test_pulse();
  (void)mln_test_wait_for_count(&probe->started, 2);

  if (submitted) {
    thread->operation_status = mln_test_completion_settle(&operation);
  }
  if (thread->create_status == MLN_STATUS_OK) {
    thread->close_status = mln_test_runtime_close(runtime);
  }
}

// Two runtimes on two threads open one cache database and run an operation on
// it at the same time.
static void two_runtimes_can_use_the_same_cache_database(void) {
  char cache_path[1024];
  mln_test_temp_path("shared-cache.db", cache_path, sizeof(cache_path));
  shared_cache_probe probe = {.cache_path = cache_path};
  atomic_init(&probe.started, 0);
  shared_cache_thread threads[2];
  mln_test_thread* handles[2];
  for (int index = 0; index < 2; index += 1) {
    threads[index] = (shared_cache_thread){
      .probe = &probe,
      .index = index,
      .create_status = MLN_STATUS_NATIVE_ERROR,
      .operation_status = MLN_STATUS_NATIVE_ERROR,
      .close_status = MLN_STATUS_NATIVE_ERROR,
    };
    handles[index] = mln_test_thread_start(touch_shared_cache, &threads[index]);
  }
  for (int index = 0; index < 2; index += 1) {
    mln_test_thread_join(handles[index]);
  }
  (void)remove(cache_path);
  for (int index = 0; index < 2; index += 1) {
    MLN_TEST_OK(threads[index].create_status);
    MLN_TEST_OK(threads[index].operation_status);
    MLN_TEST_OK(threads[index].close_status);
  }
}

MLN_TEST_GROUP {
  RUN_TEST(ambient_cache_calls_validate_their_arguments);
  RUN_TEST(ambient_cache_changes_reach_the_next_request);
  RUN_TEST(two_runtimes_can_use_the_same_cache_database);
}
