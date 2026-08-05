// The runtime and the map, owned for their whole lifetime by the runtime loop
// thread.

#ifndef C_MAP_MAP_STATE_H
#define C_MAP_MAP_STATE_H

#include <maplibre_native_c.h>

#include "channel.h"
#include "types.h"

typedef struct map_state {
  mln_runtime runtime;
  mln_map map;
} map_state;

[[nodiscard]] app_error map_state_init(
  map_state* out_state, viewport initial_viewport
);
void map_state_deinit(map_state* state);

/// Applies every queued camera command on the map's owner thread. `batch` is
/// owned by the runtime loop and reused across drains.
[[nodiscard]] app_error map_state_apply_commands(
  map_state* state, command_queue* commands, command_list* batch
);

/// Drains runtime events, reporting whether the map wants another frame.
[[nodiscard]] app_error map_state_drain_events(
  map_state* state, bool* out_render_update
);

#endif  // C_MAP_MAP_STATE_H
