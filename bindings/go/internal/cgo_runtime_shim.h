#ifndef MLN_GO_INTERNAL_CGO_RUNTIME_SHIM_H
#define MLN_GO_INTERNAL_CGO_RUNTIME_SHIM_H

#include "maplibre_native_c.h"

// Test constructors write payload unions before the generated decoder reads
// them.

static inline mln_runtime_event mln_go_runtime_event_with_render_frame(
  mln_runtime_event event, mln_runtime_event_render_frame payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
  event.payload.render_frame = payload;
  return event;
}

static inline mln_runtime_event mln_go_runtime_event_with_render_map(
  mln_runtime_event event, mln_runtime_event_render_map payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
  event.payload.render_map = payload;
  return event;
}

static inline mln_runtime_event mln_go_runtime_event_with_tile_action(
  mln_runtime_event event, mln_runtime_event_tile_action payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION;
  event.payload.tile_action = payload;
  return event;
}

static inline mln_runtime_event mln_go_runtime_event_with_offline_region_status(
  mln_runtime_event event, mln_runtime_event_offline_region_status payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS;
  event.payload.offline_region_status = payload;
  return event;
}

static inline mln_runtime_event
mln_go_runtime_event_with_offline_region_response_error(
  mln_runtime_event event,
  mln_runtime_event_offline_region_response_error payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR;
  event.payload.offline_region_response_error = payload;
  return event;
}

static inline mln_runtime_event
mln_go_runtime_event_with_offline_region_tile_count_limit(
  mln_runtime_event event,
  mln_runtime_event_offline_region_tile_count_limit payload
) {
  event.payload_type =
    MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT;
  event.payload.offline_region_tile_count_limit = payload;
  return event;
}

static inline mln_runtime_event
mln_go_runtime_event_with_camera_transition_finished(
  mln_runtime_event event, mln_runtime_event_camera_transition_finished payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED;
  event.payload.camera_transition_finished = payload;
  return event;
}

#endif
