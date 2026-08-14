#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_frame_demand_default() noexcept -> mln_frame_demand {
  return mln_frame_demand{
    .size = sizeof(mln_frame_demand),
    .flags = MLN_FRAME_DEMAND_IF_NEEDED,
    .token = 0,
    .coalescing_boundary = 0,
    .presentation_time_ns = 0,
    .deadline_ns = 0,
  };
}
auto mln_render_session_attach_options_default() noexcept
  -> mln_render_session_attach_options {
  return mln_render_session_attach_options{
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .requested_texture_ring_depth = 1,
    .reserved = 0,
    .operation_source = MLN_HANDLE_NULL,
    .frame_source = MLN_HANDLE_NULL,
    .driver_work_source = MLN_HANDLE_NULL,
  };
}

auto mln_gpu_sync_default() noexcept -> mln_gpu_sync {
  return mln_gpu_sync{
    .size = sizeof(mln_gpu_sync),
    .kind = MLN_GPU_SYNC_CPU_COMPLETE,
    .object = nullptr,
    .value = 0,
  };
}

auto mln_render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_get_capabilities(session, out);
  });
}

auto mln_render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_get_snapshot(session, out);
  });
}

auto mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_request_frame(session, demand);
  });
}

auto mln_render_session_drain_frame_results(
  mln_render_session session, size_t maximum, mln_render_frame_batch* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_drain_frame_results(session, maximum, out);
  });
}

auto mln_render_frame_batch_count(
  mln_render_frame_batch batch, size_t* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_frame_batch_count(batch, out);
  });
}

auto mln_render_frame_batch_get(
  mln_render_frame_batch batch, size_t index, mln_render_frame_result* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_frame_batch_get(batch, index, out);
  });
}

void mln_render_frame_batch_release(mln_render_frame_batch batch) noexcept {
  mln::core::render_frame_batch_release(batch);
}

auto mln_render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_acquire_frame(session, out);
  });
}

auto mln_acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::acquired_frame_get_result(frame, out);
  });
}

auto mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::acquired_frame_get_producer_sync(frame, out);
  });
}

auto mln_acquired_frame_release_start(
  mln_acquired_frame* frame, const mln_gpu_sync* sync, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::acquired_frame_release_start(frame, sync, out);
  });
}

auto mln_render_session_resize_start(
  mln_render_session session, const mln_render_target_extent* extent,
  mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_resize_start(session, extent, out);
  });
}

auto mln_render_session_barrier_start(
  mln_render_session session, uint64_t generation, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_barrier_start(session, generation, out);
  });
}

auto mln_render_session_reduce_memory_use_start(
  mln_render_session session, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_maintenance_start(session, 0, out);
  });
}

auto mln_render_session_clear_data_start(
  mln_render_session session, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_maintenance_start(session, 1, out);
  });
}

auto mln_render_session_dump_debug_logs_start(
  mln_render_session session, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_maintenance_start(session, 2, out);
  });
}

auto mln_render_session_set_feature_state_start(
  mln_render_session session, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_buffer_view state, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_set_feature_state_start(
      session, source, layer, feature, state, out
    );
  });
}

auto mln_render_session_get_feature_state_start(
  mln_render_session session, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_get_feature_state_start(
      session, source, layer, feature, out
    );
  });
}

auto mln_render_session_get_feature_state_take_result(
  mln_operation operation, mln_buffer* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_get_feature_state_take_result(
      operation, out
    );
  });
}

auto mln_render_session_remove_feature_state_start(
  mln_render_session session, mln_buffer_view source, mln_buffer_view layer,
  mln_buffer_view feature, mln_buffer_view key, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_remove_feature_state_start(
      session, source, layer, feature, key, out
    );
  });
}

auto mln_render_session_service_driver_work(
  mln_render_session session, size_t maximum, size_t* serviced
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_service_driver_work(
      session, maximum, serviced
    );
  });
}

auto mln_render_session_detach_start(
  mln_render_session session, mln_operation* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_detach_start(session, out);
  });
}

auto mln_render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_abandon(session, out);
  });
}

auto mln_render_session_destroy(mln_render_session session) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&] {
    return mln::core::render_session_destroy(session);
  });
}
