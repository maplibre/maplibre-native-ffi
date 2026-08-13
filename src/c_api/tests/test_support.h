#ifndef MLN_C_API_TEST_SUPPORT_H
#define MLN_C_API_TEST_SUPPORT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MLN_BUFFER_LITERAL(literal) \
  ((mln_buffer_view){.data = (literal), .size = sizeof(literal) - 1})

static inline mln_buffer_view mln_test_buffer_view(
  const void* data, size_t size
) {
  return (mln_buffer_view){.data = data, .size = size};
}

typedef struct mln_test_render_fixture {
  mln_render_session session;
  void* backend_state;
} mln_test_render_fixture;

// Opaque host thread used by tests that need a second native thread.
typedef struct mln_test_thread mln_test_thread;

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument);
void mln_test_thread_join(mln_test_thread* thread);
void mln_test_sleep_milliseconds(unsigned int milliseconds);
// Monotonic milliseconds for bounded concurrency assertions.
uint64_t mln_test_monotonic_milliseconds(void);
// Synthetic operation and endpoint controls exercise completion and
// notification races without depending on one MapLibre file-source schedule.
typedef struct mln_test_operation_control mln_test_operation_control;
typedef struct mln_test_endpoint_control mln_test_endpoint_control;

mln_status mln_test_operation_create(
  mln_notification_source source, bool cancellable,
  mln_operation* out_operation, mln_test_operation_control** out_control
);
mln_status mln_test_runtime_pending_operation_create(
  mln_runtime runtime, mln_operation* out_operation,
  mln_test_operation_control** out_control
);
mln_status mln_test_runtime_reserve_child(mln_runtime runtime);
void mln_test_runtime_abandon_child(mln_runtime runtime);
void mln_test_operation_complete(
  mln_test_operation_control* control, mln_status status, const char* diagnostic
);
unsigned int mln_test_operation_cancel_count(
  const mln_test_operation_control* control
);
void mln_test_operation_block_cancel(
  mln_test_operation_control* control, atomic_bool* entered,
  const atomic_bool* release
);
void mln_test_operation_control_destroy(mln_test_operation_control* control);

mln_status mln_test_endpoint_create(
  mln_notification_source source, uint64_t id, uint32_t kind, bool sticky,
  mln_test_endpoint_control** out_control
);
void mln_test_endpoint_mark_ready(mln_test_endpoint_control* control);
void mln_test_endpoint_clear_ready(mln_test_endpoint_control* control);
void mln_test_endpoint_control_destroy(mln_test_endpoint_control* control);

// Holds the source's real ready-drain lease until release is set.
void mln_test_hold_notification_ready_drain(
  mln_notification_source source, atomic_bool* entered,
  const atomic_bool* release
);

// These helpers track what they create per calling thread so the suite can
// reclaim handles a test left behind. The matching destroy helpers untrack.
mln_runtime mln_test_create_runtime(void);
mln_status mln_test_runtime_barrier(mln_runtime runtime);
mln_status mln_test_runtime_create(
  const mln_runtime_options* options, mln_runtime* out_runtime
);
mln_status mln_test_runtime_close(mln_runtime runtime);
mln_map mln_test_create_map(mln_runtime runtime);
mln_map mln_test_create_map_with_options(
  mln_runtime runtime, const mln_map_options* options
);
mln_status mln_test_map_create_status(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
);
mln_status mln_test_map_close(mln_map map);
mln_status mln_test_map_get_size(
  mln_map map, uint32_t* out_width, uint32_t* out_height,
  double* out_scale_factor
);
mln_status mln_test_map_get_event_mask(mln_map map, uint64_t* out_mask);
mln_status mln_test_map_get_camera(mln_map map, mln_camera_options* out_camera);
mln_status mln_test_map_request_repaint(mln_map map);
mln_status mln_test_map_set_event_mask(mln_map map, uint64_t mask);
mln_status mln_test_map_set_style_json(mln_map map, mln_buffer_view json);
mln_status mln_test_map_set_style_url(mln_map map, const char* url);
mln_status mln_test_map_copy_loaded_style_json(
  mln_map map, char* out, size_t capacity, size_t* out_size
);
mln_status mln_test_map_copy_style_url(
  mln_map map, char* out, size_t capacity, size_t* out_size
);
void mln_test_destroy_runtime(mln_runtime runtime);
void mln_test_destroy_map(mln_map map);
void mln_test_sleep_millisecond(void);

// Reads relative_path under the directory named by MLN_FFI_TEST_FIXTURE_DIR,
// which ctest sets. Returns malloc'd bytes the caller frees and sets *out_size,
// or null when the file cannot be read.
uint8_t* mln_test_read_fixture(const char* relative_path, size_t* out_size);

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

// Outcome of building the dedicated EGL surface fixture. Unavailable is a skip;
// a failed attach is a failure, because it is the behavior under test.
typedef enum mln_test_fixture_result {
  MLN_TEST_FIXTURE_OK,
  MLN_TEST_FIXTURE_UNAVAILABLE,
  MLN_TEST_FIXTURE_ATTACH_FAILED,
} mln_test_fixture_result;

// Attaches an OpenGL surface session that owns its EGL context, presenting into
// a pbuffer this helper creates.
mln_test_fixture_result mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture);
bool mln_test_egl_context_is_current(void);
// Waits until `flag` is set while draining runtime events. Returns whether the
// flag was observed before the bounded deadline.
bool mln_test_wait_until(mln_runtime runtime, atomic_bool* flag);

typedef struct mln_test_event_batch {
  uint32_t size;
  uint32_t event_size;
  const mln_runtime_event* events;
  size_t event_count;
  const char* messages;
  size_t messages_size;
  size_t remaining_count;
} mln_test_event_batch;

mln_test_event_batch mln_test_event_batch_default(void);
mln_status mln_test_drain_events(
  mln_runtime runtime, size_t max_events, mln_test_event_batch* out_batch
);
// Drains until a batch reports no events, and returns how many it discarded. A
// bounded batch still empties the queue, because each drain reports what stayed
// behind.
size_t mln_test_drain_all(mln_runtime runtime);

// The same, counting the events whose type matches.
size_t mln_test_drain_counting(mln_runtime runtime, uint32_t type);

// Drains until one event of `type` from `source` is found, copies it and its
// message out of the batch, and discards the rest. Pass MLN_HANDLE_NULL as
// `source` to match any source. The message is written null-terminated and
// truncated to message_capacity. Returns whether a match was found.
bool mln_test_drain_find(
  mln_runtime runtime, uint32_t type, mln_map source,
  mln_runtime_event* out_event, char* out_message, size_t message_capacity
);

// Holds the runtime event queue's real drain lease until `release` is set.
void mln_test_hold_runtime_event_drain(
  mln_runtime runtime, atomic_bool* entered, const atomic_bool* release
);

// Destroys everything this thread still has tracked, render session first, then
// map, then runtime, and reports whether it reclaimed anything. Reports through
// its return value rather than assertions, which would longjmp out of teardown.
bool mln_test_reclaim_thread_resources(void);

// Releases the graphics device this thread cached. Every thread must call this
// before its entry function returns: on browser WebGPU a live GPUDevice pins an
// Emscripten keepalive and pthread_join on that thread blocks forever. A no-op
// where the backend caches nothing per thread.
void mln_test_release_thread_gpu_resources(void);

#ifdef __cplusplus
}
#endif

#endif
