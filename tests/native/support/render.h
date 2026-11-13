#ifndef MLN_NATIVE_TESTS_RENDER_H
#define MLN_NATIVE_TESTS_RENDER_H

// The backend-neutral render fixtures. An offscreen fixture renders on a
// device that its session creates, and reads back only. A borrowed fixture
// renders into a ring of textures that the fixture creates on a host device
// for the preset's backend, and hands each frame to the case. The host device
// comes from tests/graphics through render_graphics.c.

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

// Asks a borrowed fixture for the preset's driver: the caller-driver for an
// OpenGL or WebGPU device, and the core worker for a Vulkan or Metal one.
#define MLN_TEST_PRESET_DRIVER UINT32_MAX

// Attaches a 64x64 offscreen session with no device, on the core worker, so
// the session creates its own device. It renders and reads back, and hands no
// frames to the case.
bool mln_test_render_fixture_create_offscreen(
  mln_map map, mln_test_render_fixture* fixture
);

// Attaches a session on the preset's driver to a borrowed ring of two 64x64
// RGBA8 textures that the fixture creates and owns. The case acquires its
// frames, and cannot read them back through the session.
bool mln_test_render_fixture_create_borrowed(
  mln_map map, mln_test_render_fixture* fixture
);

// Attaches a session on `driver`, or on the preset's driver for
// MLN_TEST_PRESET_DRIVER, to a borrowed ring of `depth` textures, up to
// MLN_TEXTURE_RING_MAX_DEPTH, that the fixture creates and owns.
bool mln_test_render_fixture_create_borrowed_ring(
  mln_map map, mln_test_render_fixture* fixture, size_t depth, uint32_t driver
);

void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

// Submits a borrowed attach with the caller's options, whose driver it
// replaces with the preset's, and the caller's completion, and returns without
// waiting for the attach. The ring has two textures. Returns the submission's
// status, or MLN_STATUS_NATIVE_ERROR, submitting nothing, when the backend
// cannot create the ring. On MLN_STATUS_OK the fixture holds the attaching
// session, which the case retires; mln_test_render_fixture_destroy() then
// releases the backend state, and detaches and destroys the session if it is
// still live.
mln_status mln_test_render_fixture_start_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
);

// mln_test_render_fixture_start_attach() for a 64x64 offscreen session, which
// a core worker drives.
mln_status mln_test_render_fixture_start_offscreen_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
);

// Services driver work when the fixture's session uses the caller-driver, and
// reports the service status. A core-worker session needs no service.
mln_status mln_test_render_fixture_service(
  const mln_test_render_fixture* fixture
);

// What a helper below returns when its deadline passes. No library call
// returns it.
#define MLN_TEST_RENDER_TIMED_OUT ((mln_status)INT32_MIN)

// Services the fixture's driver work until `ready(context)` holds, waiting for
// the fixture's wakes in between with mln_test_await_on_event_loop(), so a
// WebGPU readback's map reaches the thread that services. Returns
// MLN_STATUS_OK once it holds, the service status when service fails, or
// MLN_TEST_RENDER_TIMED_OUT at the deadline.
mln_status mln_test_render_step_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, mln_test_deadline deadline, const char* what
);

// Services the fixture until `completion` is delivered, and returns its
// status, or MLN_TEST_RENDER_TIMED_OUT when it never arrives.
mln_status mln_test_render_fixture_finish_operation(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
);

// Submits a session operation through `expression`, which passes
// `&completion.descriptor`, services the fixture until it completes, and
// expects its terminal status.
#define MLN_TEST_RENDER_AWAIT(expected_status, fixture, expression)          \
  do {                                                                       \
    mln_test_completion completion = mln_test_completion_default(0);         \
    TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, (expression), #expression); \
    TEST_ASSERT_EQUAL_INT_MESSAGE(                                           \
      (expected_status),                                                     \
      mln_test_render_fixture_finish_operation((fixture), &completion),      \
      #expression                                                            \
    );                                                                       \
    mln_test_completion_destroy(&completion);                                \
  } while (false)

// The extent of `size` at the scale factor that `session` renders at now, for
// a resize that keeps it.
mln_logical_extent mln_test_session_extent(
  mln_render_session session, mln_logical_size size
);

// Returns the status of the error that made `session` fail, MLN_STATUS_OK
// while it hasn't failed, and copies the error's message into `message`, which
// holds `capacity` bytes, when that is nonzero.
mln_status mln_test_render_session_failure(
  mln_render_session session, char* message, size_t capacity
);

// Waits for `completion`, the disposal's of the fixture's session, servicing
// the session meanwhile when it uses the caller-driver, and returns its
// status, or MLN_TEST_RENDER_TIMED_OUT when it never arrives.
mln_status mln_test_render_fixture_finish_disposal(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
);

// Releases `session` and waits until it retires, servicing it meanwhile when
// it uses the caller-driver. Returns the completion's status, or the status
// that rejected the release.
mln_status mln_test_render_session_release(mln_render_session session);

// Releases the graphics device this thread cached. Every thread must call this
// before its entry function returns. A no-op where the backend caches nothing
// per thread.
void mln_test_release_thread_gpu_resources(void);

// Destroys the render sessions this thread still has tracked and reports
// whether there were any. Part of mln_test_reclaim_thread_resources().
bool mln_test_render_reclaim_thread_sessions(void);

#ifdef __cplusplus
}
#endif

#endif
