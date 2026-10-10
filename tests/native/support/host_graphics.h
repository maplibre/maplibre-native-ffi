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

// The deepest borrowed ring a fixture attaches or replaces.
#define MLN_TEST_MAX_RING_DEPTH 3U

// Asks mln_test_render_fixture_create_borrowed_ring() for the preset's driver.
#define MLN_TEST_PRESET_DRIVER UINT32_MAX

// Attaches a session to a borrowed ring of one texture that the fixture
// creates, with the preset's driver. The fixture owns the texture until it is
// destroyed.
bool mln_test_render_fixture_create_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture
);

// Attaches a session on `driver` to a borrowed ring of `depth` textures, up to
// MLN_TEST_MAX_RING_DEPTH, that the fixture creates and owns.
bool mln_test_render_fixture_create_borrowed_ring(
  mln_map map, mln_test_render_fixture* fixture, size_t depth, uint32_t driver
);

// The texture at `index` of a borrowed fixture's ring, or null past its end.
mln_test_graphics_texture* mln_test_render_fixture_texture(
  const mln_test_render_fixture* fixture, size_t index
);

// The handle that names `texture` to the preset's backend: the
// id<MTLTexture>, the VkImage, or the OpenGL texture name.
uint64_t mln_test_texture_handle(const mln_test_graphics_texture* texture);

// Copies the preset backend's metadata from `frame`, and returns the handle of
// the texture it names and, through `out_slot`, its ring slot.
uint64_t mln_test_frame_texture_handle(
  mln_acquired_frame frame, uint32_t* out_slot
);

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Receives the context's PFN_vkGetDeviceProcAddr and returns the one the
// session resolves its device functions through.
typedef void* (*mln_test_vulkan_device_proc_addr_wrap)(
  void* get_device_proc_addr
);

// mln_test_render_fixture_create_borrowed_ring() on the preset's driver for a
// session whose context names `wrap`'s vkGetDeviceProcAddr, so that a case can
// observe the device functions the session calls. With a non-null `share`,
// the session attaches on the device and queue of that fixture, which must
// outlive this one. A non-null `queue_lock` is the session's host queue lock.
bool mln_test_render_fixture_create_vulkan_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture, size_t depth,
  mln_test_vulkan_device_proc_addr_wrap wrap,
  const mln_test_render_fixture* share, const mln_queue_lock* queue_lock
);

// mln_test_render_fixture_create() for a session whose context names
// `wrap`'s vkGetDeviceProcAddr, when `wrap` is non-null, and with the host
// queue lock `queue_lock`, when that is non-null.
bool mln_test_render_fixture_create_vulkan_owned_texture(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_vulkan_device_proc_addr_wrap wrap, const mln_queue_lock* queue_lock
);

// mln_test_render_fixture_create_surface() for a session whose context names
// `wrap`'s vkGetDeviceProcAddr.
bool mln_test_render_fixture_create_vulkan_surface(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_vulkan_device_proc_addr_wrap wrap
);
#endif

// Attaches a session to a presentation surface that the fixture creates, with
// the preset's driver.
bool mln_test_render_fixture_create_surface(
  mln_map map, mln_test_render_fixture* fixture
);

// Copies the first texture of a borrowed fixture's ring, as described for
// mln_test_graphics_texture_read_rgba8().
bool mln_test_render_fixture_read_texture(
  const mln_test_render_fixture* fixture, uint8_t* pixels, size_t size
);

// Makes mln_test_render_fixture_destroy() leave the fixture's graphics object
// and targets alive until the process exits, as a host does once abandon or
// detach keeps a Vulkan object: the kept object is a child of the host's
// device, and a kept swapchain of its surface too, so destroying either is
// undefined behavior.
void mln_test_render_fixture_keep_graphics_until_exit(
  const mln_test_render_fixture* fixture
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
// fixture's session, naming the `count` `textures`, up to
// MLN_TEST_MAX_RING_DEPTH, or `surface`, and the context of `graphics`, which
// created them. Returns the submission status, or MLN_STATUS_NATIVE_ERROR when
// the target cannot be described.
mln_status mln_test_render_fixture_set_textures(
  const mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  mln_test_graphics_texture* const* textures, size_t count,
  const mln_completion* completion
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
