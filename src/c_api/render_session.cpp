#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

auto mln_render_session_create_projection(
  mln_render_session session, mln_map_projection* out_projection,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_projection_create(session, out_projection);
  });
}

auto mln_frame_demand_default() noexcept -> mln_frame_demand {
  return mln_frame_demand{
    .size = sizeof(mln_frame_demand),
    .flags = MLN_FRAME_DEMAND_IF_NEEDED,
    .token = 0,
    .coalescing_boundary = 0,
    .timeout_ns = 0,
  };
}
auto mln_render_session_attach_options_default() noexcept
  -> mln_render_session_attach_options {
  return mln_render_session_attach_options{
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    .requested_texture_ring_depth = 1,
    .reserved = 0,
    .frame_wake = mln_wake{},
    .driver_work_wake = mln_wake{},
    .queue_lock = mln_queue_lock{},
  };
}

auto mln_gpu_sync_default() noexcept -> mln_gpu_sync {
  return mln_gpu_sync{
    .size = sizeof(mln_gpu_sync),
    .kind = MLN_GPU_SYNC_CPU_COMPLETE,
    .object = 0,
    .value = 0,
  };
}

auto mln_render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out_capabilities,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_get_capabilities(
      session, out_capabilities
    );
  });
}

auto mln_render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out_snapshot,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_get_snapshot(session, out_snapshot);
  });
}

auto mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_request_frame(session, demand);
  });
}

auto mln_render_session_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_drain_frame_results(session, out_batch);
  });
}

auto mln_render_frame_batch_get(
  mln_render_frame_batch batch, mln_render_frame_batch_view* out_view,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_frame_batch_get(batch, out_view);
  });
}

void mln_render_frame_batch_release(mln_render_frame_batch batch) noexcept {
  mln::core::render_frame_batch_release(batch);
}

auto mln_render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_acquire_frame(session, out_frame);
  });
}

auto mln_acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_get_result(frame, out_result);
  });
}

auto mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_get_producer_sync(frame, out_sync);
  });
}

auto mln_acquired_frame_release(
  mln_acquired_frame* frame, const mln_gpu_sync* consumer_completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_release(frame, consumer_completion);
  });
}

auto mln_render_session_resize(
  mln_render_session session, mln_logical_extent extent,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_resize_start(session, extent, completion);
  });
}

auto mln_render_session_barrier(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_barrier_start(session, completion);
  });
}

auto mln_render_session_reduce_memory_use(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::ReduceMemoryUse, completion
    );
  });
}

auto mln_render_session_clear_data(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::ClearData, completion
    );
  });
}

auto mln_render_session_dump_debug_logs(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_maintenance_start(
      session, mln::core::RenderSessionMaintenance::DumpDebugLogs, completion
    );
  });
}

auto mln_render_session_service_driver_work(
  mln_render_session session, size_t max_work, size_t* out_serviced,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_service_driver_work(
      session, max_work, out_serviced
    );
  });
}

auto mln_render_session_detach(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_detach_start(session, completion);
  });
}

auto mln_render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_abandon(session, out_result);
  });
}

auto mln_render_session_destroy(
  mln_render_session session, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_destroy(session);
  });
}

auto mln_render_session_dispose(
  mln_render_session session, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::render_session_dispose(session);
  });
}

auto mln_acquired_frame_dispose(
  mln_acquired_frame frame, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_dispose(frame);
  });
}

auto mln_acquired_frame_view_begin(
  mln_acquired_frame frame, void** out_scope, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&] {
    return mln::core::acquired_frame_view_begin(frame, out_scope);
  });
}
void mln_acquired_frame_view_end(void* scope) noexcept {
  mln::core::acquired_frame_view_end(scope);
}
