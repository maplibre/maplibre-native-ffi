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

// Inline style documents the suite loads so a test never reaches the network.
// The empty one parses with no layer, the background one paints one opaque
// layer, and the red one paints a layer a readback can recognize.
extern const mln_buffer_view mln_test_empty_style_json;
extern const mln_buffer_view mln_test_background_style_json;
extern const mln_buffer_view mln_test_red_background_style_json;

static inline mln_buffer_view mln_test_buffer_view(
  const void* data, size_t size
) {
  return (mln_buffer_view){.data = data, .size = size};
}

typedef struct mln_test_render_fixture {
  mln_render_session session;
  void* backend_state;
  uint32_t driver;
  bool observed_attaching;
  bool observed_driver_ready;
  atomic_uint frame_wakes;
  atomic_uint driver_wakes;
} mln_test_render_fixture;

// Opaque host thread used by tests that need a second native thread.
typedef struct mln_test_thread mln_test_thread;

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument);
void mln_test_thread_join(mln_test_thread* thread);
void mln_test_sleep_milliseconds(unsigned int milliseconds);
// Monotonic milliseconds for bounded concurrency assertions.
uint64_t mln_test_monotonic_milliseconds(void);
typedef struct mln_test_completion {
  mln_completion descriptor;
  void* state;
} mln_test_completion;

// Submits a command through `expression` and asserts its terminal status. The
// macro declares the `completion` the expression must pass, so an expression
// that names anything else does not compile.
#define MLN_TEST_AWAIT_COMMAND(expected_status, expression)          \
  do {                                                               \
    mln_test_completion completion = mln_test_completion_default(0); \
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, (expression));              \
    TEST_ASSERT_EQUAL_INT(                                           \
      (expected_status), mln_test_completion_finish(&completion)     \
    );                                                               \
    mln_test_completion_destroy(&completion);                        \
  } while (false)

mln_test_completion mln_test_completion_default(size_t value_size);
mln_test_completion mln_test_completion_buffer_view(void);
mln_test_completion mln_test_completion_readback(void);
mln_completion mln_test_discard_completion(void);
void mln_test_completion_destroy(mln_test_completion* completion);
void mln_test_completion_reject(mln_test_completion* completion);
// Waits for the completion to be delivered and reports whether it arrived. A
// negative timeout_ms still stops at a bounded ceiling, so a completion the
// code under test never settles fails a named test rather than hanging the
// suite.
bool mln_test_completion_wait(
  mln_test_completion* completion, int64_t timeout_ms
);
mln_status mln_test_completion_finish(mln_test_completion* completion);
// Waits for the completion, destroys it, and reports its terminal status.
mln_status mln_test_completion_settle(mln_test_completion* completion);
// The same, copying value_size bytes of the completion's value into out_value
// first. Reports MLN_STATUS_NATIVE_ERROR when the copy fails.
mln_status mln_test_completion_finish_value(
  mln_test_completion* completion, void* out_value, size_t value_size
);
bool mln_test_completion_poll(mln_test_completion* completion);
mln_status mln_test_completion_status(mln_test_completion* completion);
uint32_t mln_test_completion_disposition(mln_test_completion* completion);
uint64_t mln_test_completion_generation(mln_test_completion* completion);
const char* mln_test_completion_diagnostic(mln_test_completion* completion);
size_t mln_test_completion_value_count(mln_test_completion* completion);
bool mln_test_completion_copy_value(
  mln_test_completion* completion, void* out_value, size_t value_size
);
// Exercises the completion state machine inside the library and returns the
// clause that failed, or null when every one held.
const char* mln_test_completion_contract(void);

#if defined(MLN_FFI_TEST_BACKEND_METAL)
// Returns the step that failed, or null when the retarget kept the replacement
// layer alive until the driver ran it.
const char* mln_test_metal_surface_retarget_retains_submission(mln_map map);
#endif

mln_status mln_test_render_session_blocking_operation_create(
  mln_render_session session, atomic_bool* entered, const atomic_bool* release,
  const mln_completion* completion
);

// These helpers track what they create per calling thread so the suite can
// reclaim handles a test left behind. The matching destroy helpers untrack.
mln_runtime mln_test_create_runtime(void);
mln_status mln_test_runtime_barrier(mln_runtime runtime);
mln_status mln_test_runtime_close(mln_runtime runtime);
mln_map mln_test_create_map(mln_runtime runtime);
mln_map mln_test_create_map_with_options(
  mln_runtime runtime, const mln_map_options* options
);
mln_status mln_test_map_create_status(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
);
mln_status mln_test_map_close(mln_map map);
mln_status mln_test_map_get_event_mask(mln_map map, uint64_t* out_mask);
mln_status mln_test_map_get_camera(mln_map map, mln_camera_options* out_camera);
mln_status mln_test_map_request_repaint(mln_map map);
mln_status mln_test_map_set_event_mask(mln_map map, uint64_t mask);
mln_status mln_test_map_set_style_json(mln_map map, mln_buffer_view json);
mln_status mln_test_map_set_style_url(mln_map map, const char* url);
void mln_test_destroy_runtime(mln_runtime runtime);
void mln_test_destroy_map(mln_map map);
void mln_test_sleep_millisecond(void);

// Resolves relative_path under the fixture directory into out_path. Returns
// false when out_path is too small.
bool mln_test_fixture_path(
  const char* relative_path, char* out_path, size_t out_path_capacity
);

// Reads relative_path under the fixture directory. On success, returns bytes
// that the caller frees and writes their length to out_size. Returns null when
// the file cannot be read.
uint8_t* mln_test_read_fixture(const char* relative_path, size_t* out_size);

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
// Transfers an OffscreenCanvas into a core-worker WebGL surface session.
bool mln_test_transferred_webgl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
);
#endif
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);
mln_status mln_test_render_fixture_finish_operation(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
);

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
// Attaches a private OpenGL owned texture target whose context and driver both
// belong to the native core worker.
mln_test_fixture_result mln_test_dedicated_egl_texture_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_dedicated_egl_texture_destroy(mln_test_render_fixture* fixture);
// Waits until `flag` is set while draining runtime events. Returns whether the
// flag was observed before the bounded deadline.
bool mln_test_wait_until(mln_runtime runtime, atomic_bool* flag);

// The same bounded wait without touching the C API, for a flag written by a
// callback that runs on a MapLibre thread the caller must not re-enter.
bool mln_test_wait_for_flag(const atomic_bool* flag);

// Barriers the runtime until `counter` reaches `target`, then fails naming
// `what` when the bounded deadline expires first.
void mln_test_barrier_until_count(
  mln_runtime runtime, const atomic_size_t* counter, size_t target,
  const char* what
);

// Applies an inline style and waits for the map to finish loading it, so a test
// that needs a loaded style depends on neither the network nor a fixed barrier
// count. Fails the test when the style never loads.
void mln_test_load_style_and_wait(
  mln_runtime runtime, mln_map map, mln_buffer_view json
);

typedef struct mln_test_event_batch {
  uint32_t size;
  uint32_t event_size;
  const mln_runtime_event* events;
  size_t event_count;
  const char* messages;
  size_t messages_size;
} mln_test_event_batch;

mln_test_event_batch mln_test_event_batch_default(void);
mln_status mln_test_drain_events(
  mln_runtime runtime, mln_test_event_batch* out_batch
);
// Drains until a batch reports no events, and returns how many it discarded.
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
