// Runtime and map fixtures, event draining, fixture files, and host threads.

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"

#include "render.h"
#include "unity.h"
#include "wait.h"

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";
static const char background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#102030\"}}]}";
static const char red_background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#ff0000\"}}]}";

const mln_buffer_view mln_test_empty_style_json =
  MLN_BUFFER_LITERAL(empty_style_json);
const mln_buffer_view mln_test_background_style_json =
  MLN_BUFFER_LITERAL(background_style_json);
const mln_buffer_view mln_test_red_background_style_json =
  MLN_BUFFER_LITERAL(red_background_style_json);

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

// Per-thread record of the handles these helpers created. A failing assertion
// longjmps out of the test body, so suite teardown reclaims what the test still
// holds. Tracking is thread local so one thread's teardown leaves another
// thread's runtime alone.
static MLN_TEST_THREAD_LOCAL mln_runtime tracked_runtime;
static MLN_TEST_THREAD_LOCAL mln_event_batch drained_batch;
static MLN_TEST_THREAD_LOCAL mln_map* tracked_maps;
static MLN_TEST_THREAD_LOCAL size_t tracked_map_count;
static MLN_TEST_THREAD_LOCAL size_t tracked_map_capacity;

// Grows before the handle exists: failing after creation would strand it.
static void reserve_map_slot(void) {
  if (tracked_map_count < tracked_map_capacity) {
    return;
  }
  const size_t capacity =
    tracked_map_capacity == 0 ? 8 : tracked_map_capacity * 2;
  mln_map* grown = realloc(tracked_maps, capacity * sizeof(*tracked_maps));
  TEST_ASSERT_NOT_NULL_MESSAGE(grown, "tracking a map failed");
  tracked_maps = grown;
  tracked_map_capacity = capacity;
}

static void track_map(mln_map map) {
  tracked_maps[tracked_map_count] = map;
  tracked_map_count += 1;
}

static void untrack_map(const mln_map map) {
  for (size_t index = 0; index < tracked_map_count; index += 1) {
    if (tracked_maps[index] == map) {
      tracked_maps[index] = tracked_maps[tracked_map_count - 1];
      tracked_map_count -= 1;
      return;
    }
  }
}

mln_status mln_test_runtime_close(mln_runtime runtime) {
  mln_test_completion teardown = mln_test_completion_default(0);
  const mln_status status =
    mln_runtime_release(runtime, &teardown.descriptor, MLN_TEST_DIAGNOSTIC);
  if (status != MLN_STATUS_OK) {
    // A rejected submission leaves user_data caller-owned, so release it here
    // before destroy waits for the release marker.
    mln_test_completion_reject(&teardown);
    mln_test_completion_destroy(&teardown);
    return status;
  }
  // Waiting keeps process exit ordered after native teardown, which otherwise
  // races MapLibre's process-wide singletons in short-lived hosts.
  const bool completed = mln_test_completion_wait(&teardown, -1);
  const mln_status teardown_status = mln_test_completion_status(&teardown);
  mln_test_completion_destroy(&teardown);
  if (!completed) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  return teardown_status;
}

mln_runtime mln_test_create_runtime(void) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_runtime_options options = mln_runtime_options_default();
  // Event waits block on this wake rather than polling the queue.
  options.event_wake = mln_test_pulse_wake();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  tracked_runtime = runtime;
  return runtime;
}

mln_status mln_test_map_create_status(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(*out_map));
  mln_status status = mln_map_create(
    runtime, options, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) {
    completion.descriptor.release_user_data(completion.descriptor.user_data);
    mln_test_completion_destroy(&completion);
    return status;
  }
  if (!mln_test_completion_wait(&completion, -1)) {
    status = MLN_STATUS_NATIVE_ERROR;
  } else {
    status = mln_test_completion_status(&completion);
    if (
      status == MLN_STATUS_OK &&
      !mln_test_completion_copy_value(&completion, out_map, sizeof(*out_map))
    ) {
      status = MLN_STATUS_NATIVE_ERROR;
    }
  }
  mln_test_completion_destroy(&completion);
  return status;
}

mln_status mln_test_map_close(mln_map map) {
  mln_test_completion teardown = mln_test_completion_default(0);
  const mln_status status =
    mln_map_release(map, &teardown.descriptor, MLN_TEST_DIAGNOSTIC);
  if (status != MLN_STATUS_OK) {
    mln_test_completion_reject(&teardown);
    mln_test_completion_destroy(&teardown);
    return status;
  }
  const bool completed = mln_test_completion_wait(&teardown, -1);
  const mln_status teardown_status = mln_test_completion_status(&teardown);
  mln_test_completion_destroy(&teardown);
  return completed ? teardown_status : MLN_STATUS_NATIVE_ERROR;
}

mln_status mln_test_map_get_event_mask(mln_map map, uint64_t* out_mask) {
  if (out_mask == NULL) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  const mln_status status =
    mln_map_snapshot_get(map, &snapshot, MLN_TEST_DIAGNOSTIC);
  if (status == MLN_STATUS_OK) {
    *out_mask = snapshot.event_mask;
  }
  return status;
}

mln_status mln_test_map_get_camera(
  mln_map map, mln_camera_options* out_camera
) {
  if (out_camera == NULL) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  uint64_t generation = 0;
  return mln_map_camera_snapshot_get(
    map, out_camera, &generation, MLN_TEST_DIAGNOSTIC
  );
}

static MLN_TEST_THREAD_LOCAL mln_diagnostic test_diagnostic;

mln_diagnostic* mln_test_diagnostic(void) {
  test_diagnostic.size = sizeof(test_diagnostic);
  return &test_diagnostic;
}

const char* mln_test_last_error(void) { return test_diagnostic.message; }

mln_status mln_test_map_request_repaint(mln_map map) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_map_request_repaint(map, &completion, MLN_TEST_DIAGNOSTIC);
}

mln_status mln_test_map_set_event_mask(mln_map map, uint64_t mask) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_map_set_event_mask(map, mask, &completion, MLN_TEST_DIAGNOSTIC);
}

mln_status mln_test_map_set_style_json(mln_map map, mln_buffer_view json) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_map_set_style_json(map, json, &completion, MLN_TEST_DIAGNOSTIC);
}

mln_status mln_test_map_set_style_url(mln_map map, const char* url) {
  const mln_completion completion = mln_test_discard_completion();
  return mln_map_set_style_url(map, url, &completion, MLN_TEST_DIAGNOSTIC);
}

mln_map mln_test_create_map_with_options(
  mln_runtime runtime, const mln_map_options* options
) {
  mln_map map = MLN_HANDLE_NULL;
  mln_test_completion completion = mln_test_completion_default(sizeof(map));
  reserve_map_slot();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_create(
      runtime, options, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_TRUE(mln_test_completion_wait(&completion, -1));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_status(&completion));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, &map, sizeof(map))
  );
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  track_map(map);
  return map;
}

mln_map mln_test_create_map(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  return mln_test_create_map_with_options(runtime, &options);
}

void mln_test_destroy_runtime(mln_runtime runtime) {
  mln_test_release_drained_batch();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_close(runtime));
  if (tracked_runtime == runtime) {
    tracked_runtime = MLN_HANDLE_NULL;
  }
}

mln_status mln_test_runtime_barrier(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  mln_status status =
    mln_runtime_barrier(runtime, &completion.descriptor, MLN_TEST_DIAGNOSTIC);
  if (status != MLN_STATUS_OK) {
    completion.descriptor.release_user_data(completion.descriptor.user_data);
    mln_test_completion_destroy(&completion);
    return status;
  }
  if (!mln_test_completion_wait(&completion, -1)) {
    status = MLN_STATUS_NATIVE_ERROR;
  } else {
    status = mln_test_completion_status(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

void mln_test_destroy_map(mln_map map) {
  mln_test_release_drained_batch();
  const mln_completion release = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_release(map, &release, MLN_TEST_DIAGNOSTIC)
  );
  untrack_map(map);
}

bool mln_test_fixture_path(
  const char* relative_path, char* out_path, size_t out_path_capacity
) {
#if defined(__EMSCRIPTEN__)
  // The browser suite embeds its fixtures in the module at a fixed virtual
  // path.
  const char* fixture_dir = "/fixtures";
#else
  const char* fixture_dir = getenv("MLN_FFI_TEST_FIXTURE_DIR");
  TEST_ASSERT_TRUE_MESSAGE(
    fixture_dir != NULL && fixture_dir[0] != '\0',
    "MLN_FFI_TEST_FIXTURE_DIR is unset; run the suite through ctest"
  );
#endif

  const int written =
    snprintf(out_path, out_path_capacity, "%s/%s", fixture_dir, relative_path);
  return written >= 0 && (size_t)written < out_path_capacity;
}

uint8_t* mln_test_read_fixture(const char* relative_path, size_t* out_size) {
  if (out_size != NULL) {
    *out_size = 0;
  }
  char path[1024];
  if (!mln_test_fixture_path(relative_path, path, sizeof(path))) {
    return NULL;
  }

  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  const long length = ftell(file);
  if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }

  uint8_t* bytes = malloc((size_t)length == 0 ? 1 : (size_t)length);
  if (bytes == NULL) {
    fclose(file);
    return NULL;
  }
  const size_t read_count = fread(bytes, 1, (size_t)length, file);
  fclose(file);
  if (read_count != (size_t)length) {
    free(bytes);
    return NULL;
  }
  if (out_size != NULL) {
    *out_size = read_count;
  }
  return bytes;
}

struct mln_test_thread {
  void (*entry)(void*);
  void* argument;
#if defined(_WIN32)
  HANDLE handle;
#else
  pthread_t handle;
#endif
};

// Every thread the suite starts releases its graphics device here: on browser
// WebGPU a thread that returns still holding one is never reported as exited.
#if defined(_WIN32)
static DWORD WINAPI thread_trampoline(LPVOID argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  mln_test_release_thread_gpu_resources();
  return 0;
}
#else
static void* thread_trampoline(void* argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  mln_test_release_thread_gpu_resources();
  return NULL;
}
#endif

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument) {
  mln_test_thread* thread = calloc(1, sizeof(*thread));
  TEST_ASSERT_NOT_NULL(thread);
  thread->entry = entry;
  thread->argument = argument;
#if defined(_WIN32)
  thread->handle = CreateThread(NULL, 0, thread_trampoline, thread, 0, NULL);
  TEST_ASSERT_NOT_NULL(thread->handle);
#else
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&thread->handle, NULL, thread_trampoline, thread)
  );
#endif
  return thread;
}

void mln_test_thread_join(mln_test_thread* thread) {
  if (thread == NULL) {
    return;
  }
#if defined(_WIN32)
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
#else
  pthread_join(thread->handle, NULL);
#endif
  free(thread);
}

mln_test_event_batch mln_test_event_batch_default(void) {
  mln_test_release_drained_batch();
  return (mln_test_event_batch){.size = sizeof(mln_test_event_batch)};
}

mln_status mln_test_drain_events(
  mln_runtime runtime, mln_test_event_batch* out_batch
) {
  if (out_batch == NULL || out_batch->size < sizeof(mln_test_event_batch)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mln_test_release_drained_batch();
  mln_event_batch batch = MLN_HANDLE_NULL;
  mln_status status =
    mln_runtime_drain_events(runtime, &batch, MLN_TEST_DIAGNOSTIC);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  status = mln_event_batch_get(batch, &view, MLN_TEST_DIAGNOSTIC);
  if (status != MLN_STATUS_OK) {
    mln_event_batch_release(batch);
    return status;
  }
  drained_batch = batch;
  *out_batch = (mln_test_event_batch){
    .size = sizeof(mln_test_event_batch),
    .event_size = view.event_size,
    .events = view.events,
    .event_count = view.event_count,
    .messages = view.messages,
    .messages_size = view.messages_size,
  };
  return MLN_STATUS_OK;
}

size_t mln_test_drain_all(mln_runtime runtime) {
  return mln_test_drain_counting(runtime, 0);
}

size_t mln_test_drain_counting(mln_runtime runtime, uint32_t type) {
  size_t total = 0;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(runtime, &batch) != MLN_STATUS_OK) {
      return total;
    }
    if (batch.event_count == 0) {
      return total;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      // Type 0 is not an event type, so it counts every event.
      if (type == 0 || event->type == type) {
        total += 1;
      }
    }
  }
}

bool mln_test_drain_find(
  mln_runtime runtime, uint32_t type, mln_map source,
  mln_runtime_event* out_event, char* out_message, size_t message_capacity
) {
  bool found = false;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(runtime, &batch) != MLN_STATUS_OK) {
      return found;
    }
    if (batch.event_count == 0) {
      return found;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      if (found || event->type != type) {
        continue;
      }
      if (source != MLN_HANDLE_NULL && event->source != source) {
        continue;
      }
      found = true;
      if (out_event != NULL) {
        *out_event = *event;
      }
      if (out_message != NULL && message_capacity > 0) {
        size_t copied = event->message_size;
        if (copied > message_capacity - 1) {
          copied = message_capacity - 1;
        }
        if (copied > 0) {
          memcpy(out_message, batch.messages + event->message_offset, copied);
        }
        out_message[copied] = '\0';
      }
    }
  }
}

typedef struct event_search {
  mln_runtime runtime;
  mln_test_event_match match;
  void* context;
  bool drain_failed;
} event_search;

// Drains the whole queue, as mln_test_drain_find() does, so a match leaves the
// queue empty and the events after it discarded.
static bool drain_until_match(void* context) {
  event_search* search = context;
  bool matched = false;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(search->runtime, &batch) != MLN_STATUS_OK) {
      search->drain_failed = true;
      return true;
    }
    if (batch.event_count == 0) {
      return matched;
    }
    for (size_t index = 0; index < batch.event_count && !matched; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      matched = search->match(event, batch.messages, search->context);
    }
  }
}

bool mln_test_await_event_matching(
  mln_runtime runtime, mln_test_event_match match, void* context,
  mln_test_deadline deadline
) {
  event_search search = {
    .runtime = runtime,
    .match = match,
    .context = context,
  };
  return mln_test_await(drain_until_match, &search, deadline, "an event") &&
         !search.drain_failed;
}

typedef struct event_copy {
  uint32_t type;
  mln_map source;
  mln_runtime_event* out_event;
  char* out_message;
  size_t message_capacity;
} event_copy;

static bool copy_matching_event(
  const mln_runtime_event* event, const char* messages, void* context
) {
  event_copy* copy = context;
  if (
    event->type != copy->type ||
    (copy->source != MLN_HANDLE_NULL && event->source != copy->source)
  ) {
    return false;
  }
  if (copy->out_event != NULL) {
    *copy->out_event = *event;
  }
  if (copy->out_message != NULL && copy->message_capacity > 0) {
    size_t copied = event->message_size;
    if (copied > copy->message_capacity - 1) {
      copied = copy->message_capacity - 1;
    }
    if (copied > 0) {
      memcpy(copy->out_message, messages + event->message_offset, copied);
    }
    copy->out_message[copied] = '\0';
  }
  return true;
}

bool mln_test_await_event(
  mln_runtime runtime, uint32_t type, mln_map source,
  mln_runtime_event* out_event, char* out_message, size_t message_capacity
) {
  event_copy copy = {
    .type = type,
    .source = source,
    .out_event = out_event,
    .out_message = out_message,
    .message_capacity = message_capacity,
  };
  return mln_test_await_event_matching(
    runtime, copy_matching_event, &copy, mln_test_deadline_default()
  );
}

typedef struct flag_while_draining {
  mln_runtime runtime;
  const atomic_bool* flag;
} flag_while_draining;

static bool drained_flag_is_set(void* context) {
  flag_while_draining* wait = context;
  if (atomic_load(wait->flag)) {
    return true;
  }
  // Drain so the queue does not grow without bound while the flag is unset.
  mln_test_drain_all(wait->runtime);
  return atomic_load(wait->flag);
}

bool mln_test_wait_until_deadline(
  mln_runtime runtime, const atomic_bool* flag, mln_test_deadline deadline
) {
  flag_while_draining wait = {.runtime = runtime, .flag = flag};
  return mln_test_await(drained_flag_is_set, &wait, deadline, "a flag");
}

bool mln_test_wait_until(mln_runtime runtime, const atomic_bool* flag) {
  return mln_test_wait_until_deadline(
    runtime, flag, mln_test_deadline_default()
  );
}

void mln_test_release_drained_batch(void) {
  mln_event_batch_release(drained_batch);
  drained_batch = MLN_HANDLE_NULL;
}

void mln_test_load_style_and_wait(
  mln_runtime runtime, mln_map map, mln_buffer_view json
) {
  mln_test_completion applied = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, json, &applied.descriptor, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&applied));
  // MapLibre parses an inline document and notifies its observer inside the
  // command, so the events the load produces are queued once it commits. The
  // barrier only orders this thread behind the runtime worker's own follow-up
  // work.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
}

bool mln_test_reclaim_thread_resources(void) {
  mln_test_release_drained_batch();
  // Render session before map before runtime: the C API keeps a map with a live
  // session and a runtime with live maps alive on purpose.
  bool reclaimed = mln_test_render_reclaim_thread_sessions();
  while (tracked_map_count > 0) {
    tracked_map_count -= 1;
    (void)mln_test_map_close(tracked_maps[tracked_map_count]);
    reclaimed = true;
  }
  if (tracked_runtime != MLN_HANDLE_NULL) {
    (void)mln_test_runtime_close(tracked_runtime);
    tracked_runtime = MLN_HANDLE_NULL;
    reclaimed = true;
  }
  return reclaimed;
}
