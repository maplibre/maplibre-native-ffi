// Renders one map image offscreen and copies it to host memory. The map uses
// static mode and the session owns its texture.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

typedef struct still_image_job {
  mln_runtime runtime;
  mln_map map;
  mln_render_session session;
  const char* style_url;
  void* egl_display;
  void* egl_config;
  uint32_t width;
  uint32_t height;
  atomic_bool frames_ready;
  atomic_bool completion_ready;
  bool frame_pending;
  bool rendered;
  bool reading;
  mln_status status;
  uint8_t* pixels;
  mln_texture_image_info image_info;
  void (*schedule)(void* user_data);
  void (*finished)(void* user_data, const struct still_image_job* job);
  void* user_data;
} still_image_job;

static void schedule_job(still_image_job* job) {
  job->schedule(job->user_data);
}

static void completion_finished(
  void* user_data, const mln_completion_result* result
) {
  still_image_job* job = user_data;
  job->status = result->status;
  atomic_store_explicit(&job->completion_ready, true, memory_order_release);
  schedule_job(job);
}

static void still_image_finished(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
}

static void frames_ready(void* user_data) {
  still_image_job* job = user_data;
  atomic_store_explicit(&job->frames_ready, true, memory_order_release);
  schedule_job(job);
}

static mln_status attach_owned_texture(still_image_job* job) {
  // #region attach
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = job->width;
  descriptor.extent.height = job->height;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor.context.ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  descriptor.context.data.egl.display = job->egl_display;
  descriptor.context.data.egl.config = job->egl_config;
  descriptor.context.data.egl.client_api = MLN_OPENGL_CLIENT_API_GLES;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  options.requested_texture_ring_depth = 1;
  options.frame_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = frames_ready,
    .user_data = job,
  };
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = completion_finished,
    .user_data = job,
  };
  return mln_opengl_owned_texture_attach(
    job->map, &descriptor, &options, &job->session, &completion
  );
  // #endregion attach
}

static void map_created(void* user_data, const mln_completion_result* result) {
  still_image_job* job = user_data;
  if (result->status != MLN_STATUS_OK || result->value_count != 1) {
    job->status = result->status;
    job->finished(job->user_data, job);
    return;
  }
  job->map = *(const mln_map*)result->value;
  job->status = attach_owned_texture(job);
  if (job->status != MLN_STATUS_OK) job->finished(job->user_data, job);
}

mln_status start_still_image(still_image_job* job) {
  // #region create
  mln_map_options options = mln_map_options_default();
  options.initial_extent = (mln_logical_extent){
    .width = job->width, .height = job->height, .scale_factor = 1.0
  };
  options.map_mode = MLN_MAP_MODE_STATIC;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = map_created,
    .user_data = job,
  };
  return mln_map_create(job->runtime, &options, &completion);
  // #endregion create
}

static void readback_finished(
  void* user_data, const mln_completion_result* result
) {
  // #region read
  still_image_job* job = user_data;
  job->status = result->status;
  if (result->status == MLN_STATUS_OK && result->value_count == 1) {
    const mln_texture_readback_result* readback = result->value;
    job->pixels = malloc(readback->data.size);
    if (job->pixels == NULL) {
      job->status = MLN_STATUS_NATIVE_ERROR;
    } else {
      memcpy(job->pixels, readback->data.data, readback->data.size);
      job->image_info = readback->info;
    }
  }
  job->finished(job->user_data, job);
  // #endregion read
}

static void request_frame(still_image_job* job) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.token = 1;
  job->status = mln_render_session_request_frame(job->session, &demand);
  job->frame_pending = job->status == MLN_STATUS_OK;
}

void advance_still_image(still_image_job* job) {
  if (
    atomic_load_explicit(&job->completion_ready, memory_order_acquire) &&
    job->status != MLN_STATUS_OK
  ) {
    job->finished(job->user_data, job);
    return;
  }
  if (
    atomic_exchange_explicit(
      &job->completion_ready, false, memory_order_acq_rel
    ) &&
    job->status == MLN_STATUS_OK && !job->frame_pending && !job->rendered
  ) {
    const mln_completion command = {
      .size = sizeof(mln_completion),
      .callback = still_image_finished,
      .user_data = job,
    };
    job->status = mln_map_set_style_url(job->map, job->style_url, &command);
    if (job->status == MLN_STATUS_OK) {
      atomic_store_explicit(
        &job->completion_ready, false, memory_order_release
      );
      const mln_completion still = {
        .size = sizeof(mln_completion),
        .callback = completion_finished,
        .user_data = job,
      };
      job->status = mln_map_request_still_image(job->map, &still);
    }
    if (job->status == MLN_STATUS_OK) request_frame(job);
  }

  // #region await
  if (
    atomic_exchange_explicit(&job->frames_ready, false, memory_order_acq_rel)
  ) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    if (
      mln_render_session_drain_frame_results(job->session, &batch) ==
      MLN_STATUS_OK
    ) {
      size_t count = 0;
      (void)mln_render_frame_batch_count(batch, &count);
      for (size_t index = 0; index < count; ++index) {
        mln_render_frame_result frame = {.size = sizeof(frame)};
        if (
          mln_render_frame_batch_get(batch, index, &frame) == MLN_STATUS_OK &&
          frame.token == 1
        ) {
          job->frame_pending = false;
          job->rendered = frame.disposition == MLN_RENDER_RESULT_RENDERED;
        }
      }
      mln_render_frame_batch_release(batch);
    }
  }
  if (!job->rendered && !job->frame_pending) request_frame(job);
  if (
    job->rendered &&
    atomic_load_explicit(&job->completion_ready, memory_order_acquire) &&
    !job->reading
  ) {
    job->reading = true;
    const mln_completion readback = {
      .size = sizeof(mln_completion),
      .callback = readback_finished,
      .user_data = job,
    };
    job->status = mln_texture_read_premultiplied_rgba8(job->session, &readback);
    if (job->status != MLN_STATUS_OK) job->finished(job->user_data, job);
  }
  // #endregion await
}
