// The cross-thread surface between the render loop, which owns the window and
// the render session, and the runtime loop, which owns the runtime and the map.

#ifndef C_MAP_CHANNEL_H
#define C_MAP_CHANNEL_H

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stddef.h>

#include "types.h"

typedef enum camera_command_kind : uint8_t {
  CAMERA_COMMAND_CANCEL_TRANSITIONS,
  CAMERA_COMMAND_SET_GESTURE_IN_PROGRESS,
  CAMERA_COMMAND_MOVE_BY,
  CAMERA_COMMAND_MOVE_BY_ANIMATED,
  CAMERA_COMMAND_SCALE_BY,
  CAMERA_COMMAND_SCALE_BY_ANIMATED,
  CAMERA_COMMAND_PITCH_BY,
  CAMERA_COMMAND_ADJUST_BEARING,
  CAMERA_COMMAND_ADJUST_BEARING_ANIMATED,
  CAMERA_COMMAND_ADJUST_PITCH_ANIMATED,
  CAMERA_COMMAND_RESET_ORIENTATION,
} camera_command_kind;

/// A camera change decoded on the render loop and applied on the map's owner
/// thread. Commands carry deltas rather than absolute targets, because reading
/// the camera and writing the new one has to happen together on that thread.
typedef struct camera_command {
  camera_command_kind kind;
  union {
    struct {
      bool in_progress;
    } set_gesture_in_progress;
    struct {
      double dx;
      double dy;
    } move_by;
    struct {
      double dx;
      double dy;
      double duration_ms;
    } move_by_animated;
    struct {
      double scale;
      mln_screen_point anchor;
    } scale_by;
    struct {
      double scale;
      mln_screen_point anchor;
      double duration_ms;
    } scale_by_animated;
    struct {
      double delta;
    } delta;
    struct {
      double delta;
      double duration_ms;
    } animated_delta;
    struct {
      double duration_ms;
    } reset_orientation;
  } as;
} camera_command;

typedef struct command_list {
  camera_command* items;
  size_t len;
  size_t cap;
} command_list;

void command_list_deinit(command_list* list);

/// Pending camera commands, filled by the render loop and drained by the
/// runtime loop.
///
/// The queue grows rather than dropping: a dropped delta is motion the drag
/// never gets back, and a dropped gesture bracket leaves every delta after it
/// attributed to no gesture.
typedef struct command_queue {
  SDL_Mutex* lock;
  command_list pending;
} command_queue;

void command_queue_init(command_queue* queue);
void command_queue_deinit(command_queue* queue);

/// Render loop: queues one decoded camera change. Aborts on allocation
/// failure.
void command_queue_push(command_queue* queue, camera_command command);

/// Runtime loop: hands over everything queued so far and takes `out` in
/// exchange.
void command_queue_drain_into(command_queue* queue, command_list* out);

/// One-bit signal that a frame is worth drawing.
typedef struct render_request {
  atomic_bool value;
} render_request;

void render_request_init(render_request* request);
void render_request_set(render_request* request);
bool render_request_consume(render_request* request);

/// Publishes the map and the runtime's wake source from the runtime loop to
/// the render loop, and carries shutdown and failure the other way.
///
/// The render loop uses its copy of the map handle only to attach a session,
/// which native serves from any thread; every other map call stays on the
/// runtime loop.
typedef struct map_channel {
  SDL_Mutex* lock;
  mln_map map;
  mln_wake_source wake;
  atomic_bool published;
  atomic_bool shutdown;
  atomic_bool failed;
  app_error failure;
} map_channel;

void map_channel_init(map_channel* channel);
void map_channel_deinit(map_channel* channel);

/// Runtime loop: announces the map it just created and its wake source.
void map_channel_publish(
  map_channel* channel, mln_map map, mln_wake_source wake
);

/// Render loop: releases the runtime loop's parked pump. A no-op before the
/// runtime loop has published, when there is nothing parked yet.
void map_channel_wake_runtime_loop(map_channel* channel);

/// Render loop: the map to attach against, once the runtime loop has one.
bool map_channel_try_map(map_channel* channel, mln_map* out_map);

/// Render loop: asks the runtime loop to stop. Called only after the render
/// session is closed, because the map cannot be destroyed before then.
void map_channel_request_shutdown(map_channel* channel);
bool map_channel_shutdown_requested(map_channel* channel);

/// Runtime loop: blocks until the render loop has closed its session. The map
/// cannot be destroyed before then.
void map_channel_await_shutdown(map_channel* channel);

void map_channel_fail(map_channel* channel, app_error error);

/// Returns whether a failure was recorded, writing the first one when so.
bool map_channel_failure(map_channel* channel, app_error* out_error);

#endif  // C_MAP_CHANNEL_H
