#ifndef MLN_NATIVE_TESTS_RENDER_H
#define MLN_NATIVE_TESTS_RENDER_H

// The backend-neutral render fixture: an owned texture target attached with a
// context the fixture creates for the preset's backend. On native targets the
// context comes from tests/graphics through render_graphics.c; in the browser
// it comes from render_webgl.c or render_webgpu.c, which CMake selects by
// preset.

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
  // Set in the browser to service the session on the browser main thread
  // instead of the calling thread, which returns to the main thread's event
  // loop after each service.
  bool serviced_on_main_thread;
  // Counted by the fixture's frame and driver-work wakes, which also pulse.
  atomic_uint frame_wakes;
  atomic_uint driver_wakes;
} mln_test_render_fixture;

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

// Submits the preset's owned-texture attach with the caller's options, whose
// driver it replaces with the preset's, and the caller's completion, and
// returns without waiting for the attach. Returns the submission's status, or
// MLN_STATUS_NATIVE_ERROR, submitting nothing, when the backend cannot create
// a context. On MLN_STATUS_OK the fixture holds the attaching session, which
// the case retires; mln_test_render_fixture_destroy() then releases the
// backend state, and detaches and destroys the session if it is still live.
mln_status mln_test_render_fixture_start_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
);

// Services driver work when the fixture's session uses the caller-driver, and
// reports the service status. A core-worker session needs no service.
mln_status mln_test_render_fixture_service(
  const mln_test_render_fixture* fixture
);

// Services the fixture's driver work until `ready(context)` holds, waiting for
// the fixture's wakes in between with mln_test_await_on_event_loop(), so a
// WebGPU readback's map reaches the thread that services. Returns
// MLN_STATUS_OK once it holds, the service status when service fails, or
// MLN_STATUS_NOT_READY at the deadline.
mln_status mln_test_render_step_until(
  const mln_test_render_fixture* fixture, bool (*ready)(void* context),
  void* context, mln_test_deadline deadline, const char* what
);

// Services the fixture until `completion` is delivered, and returns its
// status, or MLN_STATUS_NOT_READY when it never arrives.
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

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)
// The deepest borrowed ring that mln_test_webgpu_borrowed_ring_create()
// attaches.
#define MLN_TEST_WEBGPU_MAX_RING_DEPTH 3U

// Attaches a session to a borrowed ring of `depth` 64x64 RGBA8 textures that
// the fixture creates on the thread's device and owns.
bool mln_test_webgpu_borrowed_ring_create(
  mln_map map, mln_test_render_fixture* fixture, size_t depth
);

// The WGPUTexture at `index` of a borrowed ring fixture, or null past its end.
void* mln_test_webgpu_ring_texture(
  const mln_test_render_fixture* fixture, size_t index
);

// A WebGPU device that the browser main thread creates and owns, for a
// session that the main thread services while the suite's thread waits.
typedef struct mln_test_webgpu_main_thread_device {
  mln_webgpu_context_descriptor context;
  atomic_bool created;
  atomic_bool lost;
} mln_test_webgpu_main_thread_device;

// Creates the device on the browser main thread. Returns false, holding
// nothing, when the main thread gets no device.
bool mln_test_webgpu_main_thread_device_create(
  mln_test_webgpu_main_thread_device* device
);

// Destroys the device on the browser main thread and waits until the main
// thread reports it lost, which settles the runtime keepalive the device held
// there. Every session on the device must be gone first.
void mln_test_webgpu_main_thread_device_destroy(
  mln_test_webgpu_main_thread_device* device
);

// Attaches a 64x64 owned texture on `device`, serviced on the browser main
// thread.
bool mln_test_webgpu_main_thread_fixture_create(
  mln_map map, const mln_test_webgpu_main_thread_device* device,
  mln_test_render_fixture* fixture
);

// Copies the ring texture at `index` into a buffer on the device's queue, maps
// that buffer, and writes its tightly packed RGBA8 pixels to `pixels`, which
// holds at least `size` bytes. Returns false when the texture, the copy, or
// the map is unavailable.
bool mln_test_webgpu_read_ring_texture(
  const mln_test_render_fixture* fixture, size_t index, uint8_t* pixels,
  size_t size
);
#endif

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL)
// Attaches an OpenGL surface session that owns its EGL context, presenting into
// a pbuffer from tests/graphics. Returns false, attaching nothing, when the EGL
// objects or the attach fail.
bool mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture);
bool mln_test_egl_context_is_current(void);
// Attaches a private OpenGL owned texture target whose context and driver both
// belong to the native core worker.
bool mln_test_dedicated_egl_texture_create(
  mln_map map, mln_test_render_fixture* fixture
);
void mln_test_dedicated_egl_texture_destroy(mln_test_render_fixture* fixture);
#endif

#ifdef __cplusplus
}
#endif

#endif
