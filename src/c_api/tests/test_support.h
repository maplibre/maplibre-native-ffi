#ifndef MLN_C_API_TEST_SUPPORT_H
#define MLN_C_API_TEST_SUPPORT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c.h"

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

// Opaque host thread used by tests that need a second owner thread.
typedef struct mln_test_thread mln_test_thread;

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument);
void mln_test_thread_join(mln_test_thread* thread);
void mln_test_sleep_milliseconds(unsigned int milliseconds);
// Monotonic milliseconds, for tests that assert a pump returned promptly rather
// than sat out its timeout.
uint64_t mln_test_monotonic_milliseconds(void);

// These helpers track what they create per calling thread so the suite can
// reclaim handles a test left behind. The matching destroy helpers untrack.
mln_runtime mln_test_create_runtime(void);
mln_map mln_test_create_map(mln_runtime runtime);
mln_map mln_test_create_map_with_options(
  mln_runtime runtime, const mln_map_options* options
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
// Whether an EGL context is current on the calling thread. A dedicated session
// leaves its own current between renders; a shared one leaves none.
bool mln_test_egl_context_is_current(void);

// Pumps the runtime until `flag` is set or the deadline passes. Returns whether
// the flag was observed.
bool mln_test_pump_until(mln_runtime runtime, atomic_bool* flag);

// Destroys everything this thread still has tracked, render session first, then
// map, then runtime, and reports whether it reclaimed anything. Reports through
// its return value rather than assertions, which would longjmp out of teardown.
bool mln_test_reclaim_thread_resources(void);

// Releases the graphics device this thread cached. Every thread must call this
// before its entry function returns: on browser WebGPU a live GPUDevice pins an
// Emscripten keepalive and pthread_join on that thread blocks forever. A no-op
// where the backend caches nothing per thread.
void mln_test_release_thread_gpu_resources(void);

#endif
