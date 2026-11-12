#ifndef MLN_NATIVE_TESTS_RENDER_BACKEND_H
#define MLN_NATIVE_TESTS_RENDER_BACKEND_H

// What the preset's backend file, render_graphics.c on native targets or a
// browser file, provides to render.c, and what render.c shares with it. Only
// the support layer includes this header.

#include <stdbool.h>
#include <stddef.h>

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

// The driver that the preset's host device drives its sessions with.
uint32_t mln_test_backend_driver(void);

// The shape of an attach that a fixture makes: it creates what the target
// needs, then submits the attach with `options` and `completion`. Returns
// false, submitting nothing, when the target's objects cannot be created.
// Otherwise writes the backend state, which mln_test_backend_destroy()
// releases, and the submission's status.
typedef bool (*mln_test_backend_attach_fn)(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
);

// How many textures the borrowed ring that the fixture attaches has.
size_t mln_test_render_ring_depth(void);

// Creates a borrowed ring of 64x64 textures on the host device, as many as
// mln_test_render_ring_depth() gives, and attaches it.
bool mln_test_backend_attach_borrowed(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
);

// Releases what an attach function created. Null releases nothing.
void mln_test_backend_destroy(void* state);

// Attaches with `attach` on the preset's driver, and waits for the attach.
bool mln_test_render_fixture_create_with(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach
);

// mln_test_render_fixture_start_attach() with `attach` on `driver`.
mln_status mln_test_render_fixture_start_attach_with(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach, uint32_t driver
);

// Attaches with `attach` on `driver`, and waits for the attach.
bool mln_test_render_fixture_create_on(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach, uint32_t driver
);

// Shared with the backends that attach fixtures of their own.
void mln_test_render_count_wake(void* user_data);
void mln_test_render_reserve_session(void);
void mln_test_render_track_session(const mln_test_render_fixture* fixture);

#ifdef __cplusplus
}
#endif

#endif
