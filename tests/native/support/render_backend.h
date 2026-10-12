#ifndef MLN_NATIVE_TESTS_RENDER_BACKEND_H
#define MLN_NATIVE_TESTS_RENDER_BACKEND_H

// What the preset's backend file, render_graphics.c on native targets or a
// browser file, provides to render.c, and what render.c shares with it. Only
// the support layer includes this header.

#include <stdbool.h>

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

// The driver the backend's owned-texture fixture attaches with.
uint32_t mln_test_backend_driver(void);

// Creates a host-owned context and submits a 64x64 owned-texture attach with
// it. Returns false, submitting nothing, when the context cannot be created.
// Otherwise writes the backend state and the submission's status.
bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
);

// Releases what mln_test_backend_attach() created.
void mln_test_backend_destroy(void* state);

// The shape of mln_test_backend_attach(), for fixtures that attach another
// kind of target. The state it writes is released with
// mln_test_backend_destroy().
typedef bool (*mln_test_backend_attach_fn)(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
);

// mln_test_render_fixture_create() with another attach function.
bool mln_test_render_fixture_create_with(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_backend_attach_fn attach
);

// mln_test_render_fixture_create_with() for a session on `driver` instead of
// the preset's.
bool mln_test_render_fixture_create_with_driver(
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
