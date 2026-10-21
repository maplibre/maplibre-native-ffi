/*
 * Native functions of the map, tagged-union, and completion-result groups for
 * the TypeScript probe. Each queues its completion before it returns, with
 * records that a later C API could produce: elements wider than the binding's,
 * an unknown union tag, and, on a later call, a stride narrower than the
 * element.
 */
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c/delivery.h"

void mln_map_close(mln_map map) { (void)map; }

static mln_status complete(
  const mln_completion* completion, const void* value, uint32_t value_size,
  size_t value_count
) {
  mln_completion_result result = {
    .size = sizeof result,
    .status = MLN_STATUS_OK,
    .value_size = value_size,
    .value = value,
    .value_count = value_count,
  };
  mln_probe_queue_complete(completion, &result);
  return MLN_STATUS_OK;
}

// A tile ID followed by a member that this header does not declare.
typedef struct wide_tile_id {
  mln_tile_id id;
  double later;
} wide_tile_id;

static const wide_tile_id tiles[] = {{{1, 2}, 99}, {{3, 4}, 99}};

// Reports two wide tiles, then no tiles, then a stride too narrow for a tile.
mln_status mln_map_visible_tiles(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)out_diagnostic;
  static int calls;
  switch (calls++) {
    case 0:
      return complete(completion, tiles, sizeof tiles[0], 2);
    case 1:
      return complete(completion, NULL, 0, 0);
    default:
      return complete(completion, tiles, sizeof(mln_tile_id) - 1, 2);
  }
}

// Reports one wide tile, then a stride too narrow for a tile.
mln_status mln_map_find_tile(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)out_diagnostic;
  static int calls;
  return complete(
    completion, &tiles[1],
    calls++ == 0 ? sizeof tiles[1] : sizeof(mln_tile_id) - 1, 1
  );
}

// An event followed by a member that this header does not declare.
typedef struct wide_event {
  mln_event event;
  uint64_t later;
} wide_event;

// Reports a frame event, then an event whose tag this header does not declare,
// each in a record wider than the binding's.
mln_status mln_map_event(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)out_diagnostic;
  static int calls;
  static wide_event events[2];
  events[0].event.kind = MLN_EVENT_FRAME;
  events[0].event.payload.frame.timestamp = 2.5;
  events[1].event.kind = 7;
  memset(&events[1].event.payload, 0xab, sizeof events[1].event.payload);
  memset(&events[1].later, 0xcd, sizeof events[1].later);
  int index = calls++ == 0 ? 0 : 1;
  return complete(completion, &events[index], sizeof(wide_event), 1);
}

static mln_status unsupported(mln_diagnostic* out_diagnostic) {
  (void)out_diagnostic;
  return MLN_STATUS_UNSUPPORTED;
}

mln_status mln_map_zoom(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)completion;
  return unsupported(out_diagnostic);
}
mln_status mln_map_list_names(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)completion;
  return unsupported(out_diagnostic);
}
mln_status mln_map_reload(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)completion;
  return unsupported(out_diagnostic);
}
mln_status mln_map_refresh(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) {
  (void)map, (void)completion;
  return unsupported(out_diagnostic);
}
