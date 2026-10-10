#ifndef MLN_NATIVE_TESTS_MAP_H
#define MLN_NATIVE_TESTS_MAP_H

// Map helpers that need a render fixture: still images, which are how a static
// map reports that everything it loads has arrived and rendered.

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

// Requests one still image from a static or tile map that `fixture` renders,
// and keeps a frame demand in flight until the image completes. Returns the
// still image's terminal status, or MLN_STATUS_NOT_READY at the default
// deadline. When it returns MLN_STATUS_OK, no demand of its own is still
// pending; on the deadline or a failure, one may be.
mln_status mln_test_render_still_image(
  const mln_test_render_fixture* fixture, mln_map map
);

// The same for a still image the case already requested, whose completion is
// `still`. The case still destroys the completion.
mln_status mln_test_render_pending_still_image(
  const mln_test_render_fixture* fixture, mln_test_completion* still
);

#ifdef __cplusplus
}
#endif

#endif
