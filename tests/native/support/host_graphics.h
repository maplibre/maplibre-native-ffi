#ifndef MLN_NATIVE_TESTS_HOST_GRAPHICS_H
#define MLN_NATIVE_TESTS_HOST_GRAPHICS_H

// Render fixtures whose target the host creates: a borrowed texture or a
// presentation surface from tests/graphics, for the preset's backend. The
// browser presets have neither, because their contexts come from JavaScript.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "render.h"

#if !defined(__EMSCRIPTEN__)
#include "mln_test_graphics.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__EMSCRIPTEN__)
// The size of every fixture target, in pixels, at a scale factor of 1.
#define MLN_TEST_HOST_TARGET_SIZE 64U

// Attaches a session to a borrowed texture that the fixture creates, with the
// preset's driver. The fixture owns the texture until it is destroyed.
bool mln_test_render_fixture_create_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture
);

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Receives the context's PFN_vkGetDeviceProcAddr and returns the one the
// session resolves its device functions through.
typedef void* (*mln_test_vulkan_device_proc_addr_wrap)(
  void* get_device_proc_addr
);

// mln_test_render_fixture_create_borrowed_texture() for a session whose
// context names `wrap`'s vkGetDeviceProcAddr, so that a case can observe the
// device functions the session calls. With a non-null `share`, the session
// attaches on the device and queue of that fixture, which must outlive this
// one. A non-null `queue_lock` is the session's host queue lock.
bool mln_test_render_fixture_create_vulkan_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_vulkan_device_proc_addr_wrap wrap,
  const mln_test_render_fixture* share, const mln_queue_lock* queue_lock
);
#endif

// Attaches a session to a presentation surface that the fixture creates, with
// the preset's driver.
bool mln_test_render_fixture_create_surface(
  mln_map map, mln_test_render_fixture* fixture
);

// Copies the borrowed texture of a fixture that
// mln_test_render_fixture_create_borrowed_texture() made, as described for
// mln_test_graphics_texture_read_rgba8().
bool mln_test_render_fixture_read_texture(
  const mln_test_render_fixture* fixture, uint8_t* pixels, size_t size
);

// The graphics object whose context a fixture from this header or from
// mln_test_render_fixture_create() attached with.
mln_test_graphics* mln_test_render_fixture_graphics(
  const mln_test_render_fixture* fixture
);

// Creates another texture or surface of the host size on the fixture's
// graphics object, for a case to hand the session. The fixture destroys it
// after its session, and holds two of each at most. Returns null when the
// graphics object cannot create one or the fixture holds two already.
mln_test_graphics_texture* mln_test_render_fixture_new_texture(
  const mln_test_render_fixture* fixture
);
mln_test_graphics_surface* mln_test_render_fixture_new_surface(
  const mln_test_render_fixture* fixture
);

// Submits the backend's borrowed-texture or surface set_target for the
// fixture's session, naming `texture` or `surface` and the context of
// `graphics`, which created it. Returns the submission status, or
// MLN_STATUS_NATIVE_ERROR when the target cannot be described.
mln_status mln_test_render_fixture_set_texture(
  const mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  const mln_test_graphics_texture* texture, const mln_completion* completion
);
mln_status mln_test_render_fixture_set_surface(
  const mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  const mln_test_graphics_surface* surface, const mln_completion* completion
);
#endif

#ifdef __cplusplus
}
#endif

#endif
