#ifndef MLN_NATIVE_TESTS_RENDER_H
#define MLN_NATIVE_TESTS_RENDER_H

// The backend-neutral render fixture: an owned texture target attached with a
// context the fixture creates for the preset's backend. Each backend's context
// lives in its own render_<backend>.c, which CMake selects by preset.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "wait.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mln_test_render_fixture {
  mln_render_session session;
  void* backend_state;
  uint32_t driver;
  bool observed_attaching;
  bool observed_driver_ready;
  // Counted by the fixture's frame and driver-work wakes, which also pulse.
  atomic_uint frame_wakes;
  atomic_uint driver_wakes;
} mln_test_render_fixture;

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

// Services driver work when the fixture's session uses the caller-driver, and
// reports the service status. A core-worker session needs no service.
mln_status mln_test_render_fixture_service(
  const mln_test_render_fixture* fixture
);

// Services the fixture's driver work until `ready(context)` holds, blocking on
// the fixture's wakes in between. Returns MLN_STATUS_OK once it holds, the
// service status when service fails, or MLN_STATUS_NOT_READY at the deadline.
mln_status mln_test_render_step_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, mln_test_deadline deadline, const char* what
);

// Services the fixture until `completion` is delivered, and returns its
// status, or MLN_STATUS_NOT_READY when it never arrives.
mln_status mln_test_render_fixture_finish_operation(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
);

// Releases the graphics device this thread cached. Every thread must call this
// before its entry function returns: on browser WebGPU a live GPUDevice pins an
// Emscripten keepalive and pthread_join on that thread blocks forever. A no-op
// where the backend caches nothing per thread.
void mln_test_release_thread_gpu_resources(void);

// Destroys the render sessions this thread still has tracked and reports
// whether there were any. Part of mln_test_reclaim_thread_resources().
bool mln_test_render_reclaim_thread_sessions(void);

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
// Transfers an OffscreenCanvas into a core-worker WebGL surface session.
bool mln_test_transferred_webgl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
);
#endif

#if defined(MLN_FFI_TEST_BACKEND_METAL)
// Returns the step that failed, or null when the retarget kept the replacement
// layer alive until the driver ran it.
const char* mln_test_metal_surface_retarget_retains_submission(mln_map map);
#endif

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

#ifdef __cplusplus
}
#endif

#endif
