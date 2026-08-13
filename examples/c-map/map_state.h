// The runtime and map driven by the core-owned scheduler.

#ifndef C_MAP_MAP_STATE_H
#define C_MAP_MAP_STATE_H

#include <maplibre_native_c.h>

#include "types.h"

typedef struct map_state {
  mln_notification_source notification_source;
  mln_runtime runtime;
  mln_map map;
} map_state;

[[nodiscard]] app_error map_state_init(
  map_state* out_state, viewport initial_viewport,
  mln_notification_callback notification_callback, void* notification_user_data
);
void map_state_deinit(map_state* state);

[[nodiscard]] app_error map_state_camera_query(
  map_state* state, mln_camera_options* out_camera
);
[[nodiscard]] app_error map_state_update_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  const mln_animation_options* animation, uint32_t gesture_phase,
  uint64_t gesture_id
);
[[nodiscard]] app_error map_state_resize(map_state* state, viewport value);

/// Drains owned notification and event batches on the render-loop receiver.
[[nodiscard]] app_error map_state_drain_notifications(
  map_state* state, bool* out_render_update
);

#endif  // C_MAP_MAP_STATE_H
