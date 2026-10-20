// The runtime and map, driven by the native scheduler thread the runtime owns.

#ifndef C_MAP_MAP_STATE_H
#define C_MAP_MAP_STATE_H

#include <maplibre_native_c.h>

#include "types.h"

typedef struct map_state {
  mln_runtime runtime;
  mln_map map;
} map_state;

/// Creates the runtime and map. A smoke run loads an inline style, so it
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

#endif  // C_MAP_MAP_STATE_H
