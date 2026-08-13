// Renders one map image offscreen and copies it to host memory. The map uses
// static mode and the session owns its texture.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

typedef struct still_receiver {
  atomic_bool scheduled;
} still_receiver;

static void schedule_still_receiver(void* user_data) {
  still_receiver* receiver = user_data;
  atomic_store(&receiver->scheduled, true);
}

static mln_status wait_ok(mln_operation operation) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status != MLN_STATUS_OK || !completed) return status;
  return mln_operation_get_status(operation, &status) == MLN_STATUS_OK
           ? status
           : MLN_STATUS_NATIVE_ERROR;
}

static mln_render_session attach_owned_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t width,
  uint32_t height
) {
  // #region attach
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = *context;

  mln_render_session session = MLN_HANDLE_NULL;
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &session);
  // #endregion attach
  return status == MLN_STATUS_OK ? session : MLN_HANDLE_NULL;
}

static bool drain_still_events(
  mln_runtime runtime, mln_map map, bool* render_requested
) {
  mln_event_batch batch = MLN_HANDLE_NULL;
  if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK)
    return false;
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  bool valid = mln_event_batch_get(batch, &view) == MLN_STATUS_OK;
  for (size_t index = 0; valid && index < view.event_count; index++) {
    const char* bytes = (const char*)view.events + index * view.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;
    if (event->source != map) continue;
    if (
      event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE ||
      (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED &&
       event->payload.render_frame.needs_repaint)
    ) {
      *render_requested = true;
    } else if (
      event->type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED ||
      event->type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED
    ) {
      valid = false;
    }
  }
  mln_event_batch_release(batch);
  return valid;
}

static bool await_still_image(
  mln_notification_source notifications, mln_runtime runtime, mln_map map,
  mln_render_session session, mln_operation operation, still_receiver* receiver
) {
  bool rendered = false;
  bool render_requested = false;
  bool completed = false;

  // #region await
  while (!completed) {
    while (!atomic_exchange(&receiver->scheduled, false)) {
      nanosleep(&(struct timespec){.tv_nsec = 10000000}, NULL);
    }

    mln_ready_batch ready = MLN_HANDLE_NULL;
    if (
      mln_notification_source_drain_ready(notifications, &ready) !=
      MLN_STATUS_OK
    )
      return false;
    mln_ready_batch_view view = {.size = sizeof(mln_ready_batch_view)};
    if (mln_ready_batch_get(ready, &view) != MLN_STATUS_OK) {
      mln_ready_batch_release(ready);
      return false;
    }
    for (size_t index = 0; index < view.endpoint_count; index++) {
      const char* bytes =
        (const char*)view.endpoints + index * view.endpoint_size;
      const mln_ready_endpoint* endpoint = (const mln_ready_endpoint*)bytes;
      if (
        endpoint->kind == MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS &&
        endpoint->id == runtime &&
        !drain_still_events(runtime, map, &render_requested)
      ) {
        mln_ready_batch_release(ready);
        return false;
      }
    }
    mln_ready_batch_release(ready);

    if (render_requested) {
      render_requested = false;
      mln_render_result result = MLN_RENDER_RESULT_NO_UPDATE;
      if (mln_render_session_render_update(session, &result) != MLN_STATUS_OK) {
        return false;
      }
      rendered = rendered || result == MLN_RENDER_RESULT_RENDERED;
      if (result != MLN_RENDER_RESULT_RENDERED) render_requested = true;
    }
    if (mln_operation_poll(operation, &completed) != MLN_STATUS_OK)
      return false;
  }
  // #endregion await

  return completed && rendered && wait_ok(operation) == MLN_STATUS_OK;
}

static uint8_t* read_pixels(
  mln_render_session session, mln_texture_image_info* out_info
) {
  // #region read
  mln_texture_image_info info = mln_texture_image_info_default();
  mln_texture_read_premultiplied_rgba8(session, NULL, 0, &info);
  uint8_t* pixels = malloc(info.byte_length);
  if (pixels == NULL) return NULL;
  const mln_status status = mln_texture_read_premultiplied_rgba8(
    session, pixels, info.byte_length, &info
  );
  // #endregion read
  if (status != MLN_STATUS_OK) {
    free(pixels);
    return NULL;
  }
  *out_info = info;
  return pixels;
}

uint8_t* render_still_image(
  mln_notification_source notifications, mln_runtime runtime,
  const char* style_url, uint32_t width, uint32_t height,
  const mln_opengl_context_descriptor* context, mln_texture_image_info* out_info
) {
  still_receiver receiver;
  atomic_init(&receiver.scheduled, false);
  if (
    mln_notification_source_set_callback(
      notifications, schedule_still_receiver, &receiver
    ) != MLN_STATUS_OK
  ) {
    return NULL;
  }

  // #region create
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = width, .height = height, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_operation create = MLN_HANDLE_NULL;
  mln_map map = MLN_HANDLE_NULL;
  mln_status status = mln_map_create_start(runtime, &options, &create);
  if (status == MLN_STATUS_OK) status = wait_ok(create);
  if (status == MLN_STATUS_OK)
    status = mln_map_create_take_result(create, &map);
  mln_operation_release(create);
  if (status != MLN_STATUS_OK) {
    mln_notification_source_clear_callback(notifications);
    return NULL;
  }

  uint64_t command_id = 0;
  mln_map_set_event_mask(
    map,
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED |
      MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED |
      MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED,
    &command_id
  );
  // #endregion create

  uint8_t* pixels = NULL;
  const mln_render_session session =
    attach_owned_texture(map, context, width, height);
  mln_operation still = MLN_HANDLE_NULL;
  if (
    session != MLN_HANDLE_NULL &&
    mln_map_set_style_url(map, style_url, &command_id) == MLN_STATUS_OK &&
    mln_map_request_still_image_start(map, &still) == MLN_STATUS_OK &&
    await_still_image(notifications, runtime, map, session, still, &receiver)
  ) {
    pixels = read_pixels(session, out_info);
  }
  mln_operation_release(still);
  if (session != MLN_HANDLE_NULL) mln_render_session_destroy(session);

  mln_operation close = MLN_HANDLE_NULL;
  if (mln_map_close_start(map, &close) == MLN_STATUS_OK) (void)wait_ok(close);
  mln_operation_release(close);
  mln_notification_source_clear_callback(notifications);
  return pixels;
}
