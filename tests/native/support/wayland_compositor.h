#ifndef MLN_NATIVE_TESTS_WAYLAND_COMPOSITOR_H
#define MLN_NATIVE_TESTS_WAYLAND_COMPOSITOR_H

// A Wayland compositor that runs on a thread of the test process, in place of a
// desktop's, with one client connection that a case hands to the library. It
// offers wl_compositor and wl_shm, so a graphics driver presents through shared
// memory, and keeps the size and center pixel of each buffer that a surface
// commits.
//
// libwayland-server and libwayland-client are loaded at run time, as the
// graphics drivers load them, so the suite links neither.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mln_test_wayland mln_test_wayland;

// A buffer that a surface committed.
typedef struct mln_test_wayland_frame {
  int32_t width;
  int32_t height;
  // The center pixel, as red, green, blue, and alpha.
  uint8_t center[4];
} mln_test_wayland_frame;

// Starts the compositor and connects the client, and fails the case when it
// can't. `out_wayland` holds the compositor before anything can fail, so pass
// it to mln_test_wayland_stop() after a failure too.
void mln_test_wayland_start(mln_test_wayland** out_wayland);
// Disconnects the client and stops the compositor. Accepts null.
void mln_test_wayland_stop(mln_test_wayland* wayland);

// The client's `wl_display*`.
void* mln_test_wayland_display(const mln_test_wayland* wayland);
// Creates a `wl_surface*` on the client.
void* mln_test_wayland_create_surface(mln_test_wayland* wayland);
// Withholds the surface's frame callbacks from here on, as a compositor does
// for a surface that isn't shown. Its commits still count as frames.
void mln_test_wayland_hide_surface(mln_test_wayland* wayland, void* surface);
// Destroys a surface from mln_test_wayland_create_surface(). Accepts null.
void mln_test_wayland_destroy_surface(mln_test_wayland* wayland, void* surface);

// Waits for `surface` to commit a buffer that no earlier wait returned, and
// reports the newest one.
bool mln_test_wayland_wait_for_frame(
  mln_test_wayland* wayland, void* surface, mln_test_wayland_frame* out_frame
);

#ifdef __cplusplus
}
#endif

#endif
