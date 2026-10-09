#ifndef MLN_NATIVE_TESTS_ENV_H
#define MLN_NATIVE_TESTS_ENV_H

// Runtime and map fixtures, event draining, fixture files, and host threads.
//
// The runtime fixture installs an event wake that pulses, so the event waits
// below block until the runtime publishes rather than polling its queue.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "wait.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define MLN_TEST_THREAD_LOCAL __declspec(thread)
#else
#define MLN_TEST_THREAD_LOCAL _Thread_local
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

// The calling thread's diagnostic, which these helpers pass to every C API
// call. Tests pass MLN_TEST_DIAGNOSTIC to direct calls whose message they read.
mln_diagnostic* mln_test_diagnostic(void);
#define MLN_TEST_DIAGNOSTIC mln_test_diagnostic()
// The message the latest call that wrote the test diagnostic left behind.
const char* mln_test_last_error(void);

// These helpers track what they create per calling thread so the suite can
// reclaim handles a test left behind. The matching destroy helpers untrack.
mln_runtime mln_test_create_runtime(void);
// The same, from options the case fills in. The fixture's event wake replaces
// options->event_wake.
mln_runtime mln_test_create_runtime_with_options(mln_runtime_options options);
// Submits a runtime barrier and waits for its terminal status. A barrier
// completes once every earlier submission has a terminal result. It cannot
// fence work that the runtime worker starts on its own, such as handling a
// frame that a render session finished; a map command queued after that work
// can, because its completion runs after the work.
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

// Applies an inline style and waits for the map to finish loading it, so a test
// that needs a loaded style depends on neither the network nor a fixed barrier
// count. Fails the test when the style never loads.
void mln_test_load_style_and_wait(
  mln_runtime runtime, mln_map map, mln_buffer_view json
);

// Destroys everything this thread still has tracked, render session first, then
// map, then runtime, and reports whether it reclaimed anything. Reports through
// its return value rather than assertions, which would longjmp out of teardown.
bool mln_test_reclaim_thread_resources(void);

// Resolves relative_path under the fixture directory into out_path. Returns
// false when out_path is too small.
bool mln_test_fixture_path(
  const char* relative_path, char* out_path, size_t out_path_capacity
);

// Writes a path for `name` in the temporary directory: TMPDIR, the Windows
// temporary directory, or /tmp elsewhere when TMPDIR is unset. The path
// carries the process ID, so the files a case writes do not collide with
// another suite's run of the same case. Removes any file already at the path.
// Fails the case when the path does not fit.
void mln_test_temp_path(const char* name, char* out_path, size_t capacity);

// Reads relative_path under the fixture directory. On success, returns bytes
// that the caller frees and writes their length to out_size. Returns null when
// the file cannot be read.
uint8_t* mln_test_read_fixture(const char* relative_path, size_t* out_size);

// Opaque host thread used by tests that need a second native thread. Each one
// releases its cached graphics device before it exits.
typedef struct mln_test_thread mln_test_thread;

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument);
void mln_test_thread_join(mln_test_thread* thread);

// One drained batch, viewed in place. The batch stays alive until the next
// drain or destroy helper on this thread releases it.
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
// Releases the batch the latest drain on this thread kept alive.
void mln_test_release_drained_batch(void);
// Drains until a batch reports no events, and returns how many it discarded.
size_t mln_test_drain_all(mln_runtime runtime);

// The same, counting the events whose type matches.
size_t mln_test_drain_counting(mln_runtime runtime, uint32_t type);

// Returns true to end an event wait. `messages` is the batch's message block.
typedef bool (*mln_test_event_match)(
  const mln_runtime_event* event, const char* messages, void* context
);

// Drains the queue until `match` accepts an event, blocking on the runtime's
// event wake in between, and discards the rest of the queue. Returns false at
// the deadline or when a drain fails.
bool mln_test_await_event_matching(
  mln_runtime runtime, mln_test_event_match match, void* context,
  mln_test_deadline deadline
);

// Drains until a batch reports no events, and returns how many events `match`
// accepted. Here `match` counts an event instead of ending a wait.
size_t mln_test_drain_counting_matching(
  mln_runtime runtime, mln_test_event_match match, void* context
);

// Blocks until an event of `type` from `source` arrives, within the default
// deadline, copies it and its message out of the batch, and discards the rest
// of the queue. Pass MLN_HANDLE_NULL as `source` to match any source. The
// message is written null-terminated and truncated to message_capacity.
bool mln_test_await_event(
  mln_runtime runtime, uint32_t type, mln_map source,
  mln_runtime_event* out_event, char* out_message, size_t message_capacity
);

// Waits until `flag` is set, draining runtime events meanwhile. Returns whether
// the flag was set before the default deadline.
bool mln_test_wait_until(mln_runtime runtime, const atomic_bool* flag);
bool mln_test_wait_until_deadline(
  mln_runtime runtime, const atomic_bool* flag, mln_test_deadline deadline
);

#ifdef __cplusplus
}
#endif

#endif
