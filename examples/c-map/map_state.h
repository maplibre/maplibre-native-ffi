// The runtime and map, driven by the native scheduler thread the runtime owns.

#ifndef C_MAP_MAP_STATE_H
#define C_MAP_MAP_STATE_H

#include <maplibre_native_c.h>

#include "types.h"

typedef struct map_state {
  mln_runtime runtime;
  mln_map map;
} map_state;

/// Creates the runtime, whose event wake posts APP_EVENT_RUNTIME_EVENTS, and
/// the map. A smoke run loads an inline style instead of fetching one, so it
/// needs no network.
[[nodiscard]] app_error map_state_init(
  map_state* out_state, viewport initial_viewport, bool smoke
);
void map_state_deinit(map_state* state);

[[nodiscard]] app_error map_state_update_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  const mln_animation_options* animation, uint32_t gesture_phase
);

/// Ends any running camera transition, so a starting gesture takes over from
/// it rather than fighting it.
[[nodiscard]] app_error map_state_cancel_transitions(map_state* state);

/// Drains every queued runtime event and reports whether the map published a
/// render update.
[[nodiscard]] app_error map_state_drain_events(
  map_state* state, bool* out_render_update
);

#endif  // C_MAP_MAP_STATE_H
