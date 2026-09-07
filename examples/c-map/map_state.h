// The runtime and map driven by the core-owned scheduler.

#ifndef C_MAP_MAP_STATE_H
#define C_MAP_MAP_STATE_H

#include <maplibre_native_c.h>

#include "types.h"

typedef struct map_state {
  mln_runtime runtime;
  mln_map map;
} map_state;

[[nodiscard]] app_error map_state_init(
  map_state* out_state, viewport initial_viewport, mln_wake_callback event_wake,
  void* event_wake_user_data
);
void map_state_deinit(map_state* state);

/// Completion for accepted commands whose terminal metadata is not consumed.
const mln_completion* map_state_discarded_completion(void);

[[nodiscard]] app_error map_state_update_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  const mln_animation_options* animation, uint32_t gesture_phase
);

/// Ends any running camera transition, so a starting gesture takes over from
/// it rather than fighting it.
[[nodiscard]] app_error map_state_cancel_transitions(map_state* state);

/// Drains the owned event queue on the render-loop receiver.
[[nodiscard]] app_error map_state_drain_events(
  map_state* state, bool* out_render_update
);

#endif  // C_MAP_MAP_STATE_H
