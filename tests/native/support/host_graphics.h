#ifndef MLN_NATIVE_TESTS_HOST_GRAPHICS_H
#define MLN_NATIVE_TESTS_HOST_GRAPHICS_H

// The tests/graphics objects behind a native target's borrowed fixtures, the
// windows and layers behind its surface fixtures, and the fixtures that only
// native targets have: a presentation surface, and variants of the others for
// the preset's backend. The browser presets have none, because their contexts
// come from JavaScript.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "mln_test_graphics.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

// The size of every fixture target, in pixels, at a scale factor of 1.
#define MLN_TEST_HOST_TARGET_SIZE 64U

// Attaches a borrowed ring of one texture, whose attach options accept
// `producer_sync_kinds`. A Metal target that accepts queue order names the
// graphics object's command queue.
bool mln_test_render_fixture_create_synced(
  mln_map map, mln_test_render_fixture* fixture,
  mln_gpu_sync_kind_flags producer_sync_kinds
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
// outlive this one, through a device of its own that wraps them. A non-null
// `queue_lock` is the wrapped device's host queue lock.
bool mln_test_render_fixture_create_vulkan_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture, size_t depth,
  mln_test_vulkan_device_proc_addr_wrap wrap,
  const mln_test_render_fixture* share, const mln_queue_lock* queue_lock
);

// Attaches a borrowed ring of one Vulkan image whose frames go to `owner`
// between renders, with timeline producer synchronization where the device
// has it.
bool mln_test_render_fixture_create_vulkan_handoff_ring(
  mln_map map, mln_test_render_fixture* fixture, mln_vulkan_image_owner owner
);

// Attaches a borrowed ring of `depth` images with a depth image beside each,
// in which the session keeps each frame's depth, and whose frames hand over
// with MLN_GPU_SYNC_CPU_COMPLETE.
bool mln_test_render_fixture_create_vulkan_depth_ring(
  mln_map map, mln_test_render_fixture* fixture, size_t depth
);

// The depth image at `index` of a fixture's ring, or null past its end or for
// a ring without them.
mln_test_graphics_texture* mln_test_render_fixture_depth_texture(
  const mln_test_render_fixture* fixture, size_t index
);

// Attaches a session on the preset's driver to a borrowed ring of one texture
// that the fixture creates on `graphics`, which must outlive the fixture. The
// session renders on `device`, which `graphics` borrowed its objects from.
bool mln_test_render_fixture_create_vulkan_on(
  mln_map map, mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  mln_gpu_device device
);
#endif

// Destroys the textures of a borrowed fixture's ring now, for a case whose
// session no longer renders to them, such as after
// mln_render_session_release_target().
void mln_test_render_fixture_destroy_textures(
  const mln_test_render_fixture* fixture
);

// mln_test_render_fixture_start_attach() for a surface fixture, which the core
// worker drives.
mln_status mln_test_render_fixture_start_surface_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
);

// A window or layer that a surface fixture presents to: on Linux and FreeBSD,
// a Wayland surface on a compositor in the test process, and elsewhere the
// window or layer of a tests/graphics surface.
typedef struct mln_test_render_surface mln_test_render_surface;

// Creates a device of the preset's backend that can present to the fixtures'
// surfaces. A Vulkan build on Linux or FreeBSD picks the CPU's device, such as
// lavapipe, which presents through shared memory that the test compositor can
// map, as Mesa's EGL does when a compositor offers no dmabufs.
mln_gpu_device mln_test_create_presenting_device(void);

// Attaches a session on the core worker, on a device from
// mln_test_create_presenting_device(), to a surface that the fixture creates.
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

// The graphics object whose context a borrowed or surface fixture attached
// with, or null for an offscreen fixture.
mln_test_graphics* mln_test_render_fixture_graphics(
  const mln_test_render_fixture* fixture
);

// Creates another texture or surface of the host size on the fixture's
// graphics object, for a case to hand the session. The fixture destroys it
// after its session and before its graphics object, and holds two of each at
// most. Returns null when the graphics object cannot create one or the fixture
// holds two already.
mln_test_graphics_texture* mln_test_render_fixture_new_texture(
  const mln_test_render_fixture* fixture
);
mln_test_render_surface* mln_test_render_fixture_new_surface(
  const mln_test_render_fixture* fixture
);
// The same, at `width` by `height` pixels.
mln_test_graphics_texture* mln_test_render_fixture_new_sized_texture(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height
);
mln_test_render_surface* mln_test_render_fixture_new_sized_surface(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height
);

// Creates a surface of the host size on a surface fixture's compositor or
// graphics object, which the case destroys with
// mln_test_render_surface_destroy() once the session no longer uses it.
mln_test_render_surface* mln_test_render_surface_create(
  const mln_test_render_fixture* fixture
);
void mln_test_render_surface_destroy(
  const mln_test_render_fixture* fixture, mln_test_render_surface* surface
);

#if defined(MLN_FFI_TEST_BACKEND_METAL)
// The CAMetalLayer* of a surface.
void* mln_test_render_surface_metal_layer(
  const mln_test_render_surface* surface
);
#endif

// Submits the backend's borrowed-texture set_target, or
// mln_render_session_set_target() with a surface, for the fixture's session,
// naming the `count` `textures`, up to MLN_TEXTURE_RING_MAX_DEPTH, or
// `surface`, at their own size in logical pixels. Returns the submission
// status, or MLN_STATUS_NATIVE_ERROR when the target cannot be described.
mln_status mln_test_render_fixture_set_textures(
  const mln_test_render_fixture* fixture,
  mln_test_graphics_texture* const* textures, size_t count,
  const mln_completion* completion
);
mln_status mln_test_render_fixture_set_surface(
  const mln_test_render_fixture* fixture,
  const mln_test_render_surface* surface, const mln_completion* completion
);

#ifdef __cplusplus
}
#endif

#endif
