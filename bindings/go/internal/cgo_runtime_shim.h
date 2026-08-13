#ifndef MLN_GO_INTERNAL_CGO_RUNTIME_SHIM_H
#define MLN_GO_INTERNAL_CGO_RUNTIME_SHIM_H

#include "maplibre_native_c.h"

// cgo lowers mln_runtime_event_payload to an opaque byte array, so Go names no
// union member. The accessors below read one member of a drained event by
// value, and the constructors below return an event with one member written,
// which is how the test hook synthesizes a batch.

static inline mln_runtime_event_render_frame mln_go_runtime_event_render_frame(
  const mln_runtime_event* event
) {
  return event->payload.render_frame;
}

static inline mln_runtime_event_render_map mln_go_runtime_event_render_map(
  const mln_runtime_event* event
) {
  return event->payload.render_map;
}

static inline mln_runtime_event_tile_action mln_go_runtime_event_tile_action(
  const mln_runtime_event* event
) {
  return event->payload.tile_action;
}

static inline mln_runtime_event_offline_region_status
mln_go_runtime_event_offline_region_status(const mln_runtime_event* event) {
  return event->payload.offline_region_status;
}

static inline mln_runtime_event_offline_region_response_error
mln_go_runtime_event_offline_region_response_error(
  const mln_runtime_event* event
) {
  return event->payload.offline_region_response_error;
}

static inline mln_runtime_event_offline_region_tile_count_limit
mln_go_runtime_event_offline_region_tile_count_limit(
  const mln_runtime_event* event
) {
  return event->payload.offline_region_tile_count_limit;
}

static inline mln_runtime_event_camera_transition_finished
mln_go_runtime_event_camera_transition_finished(
  const mln_runtime_event* event
) {
  return event->payload.camera_transition_finished;
}

static inline mln_runtime_event_command_finished
mln_go_runtime_event_command_finished(const mln_runtime_event* event) {
  return event->payload.command_finished;
}

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

static inline mln_runtime_event mln_go_runtime_event_with_command_finished(
  mln_runtime_event event, mln_runtime_event_command_finished payload
) {
  event.payload_type = MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED;
  event.payload.command_finished = payload;
  return event;
}

#endif
