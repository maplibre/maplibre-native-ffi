use super::*;

#[test]
fn caller_and_core_driver_options_are_distinct() {
    let core = RenderSessionAttachOptions::core_worker(3).to_native();
    assert_eq!(core.driver, sys::MLN_RENDER_DRIVER_CORE_WORKER);
    assert_eq!(core.requested_texture_ring_depth, 3);
    assert_eq!(core.operation_source.0, 0);
    assert_eq!(core.frame_source.0, 0);
    assert_eq!(core.driver_work_source.0, 0);

    let caller = RenderSessionAttachOptions::caller_graphics_thread(2).to_native();
    assert_eq!(caller.driver, sys::MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
    assert_eq!(caller.requested_texture_ring_depth, 2);
}

#[test]
fn frame_demand_copies_pacing_and_coalescing_fields() {
    let raw = FrameDemand {
        if_needed: true,
        present: true,
        token: 9,
        coalescing_boundary: 4,
        presentation_time_ns: 12,
        deadline_ns: 15,
    }
    .to_native();
    assert_eq!(
        raw.flags,
        sys::MLN_FRAME_DEMAND_IF_NEEDED | sys::MLN_FRAME_DEMAND_PRESENT
    );
    assert_eq!(raw.token, 9);
    assert_eq!(raw.coalescing_boundary, 4);
    assert_eq!(raw.presentation_time_ns, 12);
    assert_eq!(raw.deadline_ns, 15);
}

#[test]
fn every_frozen_frame_disposition_is_preserved() {
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_RENDERED),
        FrameDisposition::Rendered
    );
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_NO_UPDATE),
        FrameDisposition::NoUpdate
    );
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_SIZE_PENDING),
        FrameDisposition::SizePending
    );
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_TARGET_NOT_READY),
        FrameDisposition::TargetNotReady
    );
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_SUPERSEDED),
        FrameDisposition::Superseded
    );
    assert_eq!(
        disposition_from_native(sys::MLN_RENDER_RESULT_DEADLINE_MISSED),
        FrameDisposition::DeadlineMissed
    );
}

#[test]
fn transferred_webgl_canvas_is_a_core_worker_descriptor() {
    let descriptor = WebGlContextDescriptor::transferred_canvas("#render-canvas");
    let raw = descriptor.to_core();
    assert_eq!(raw.kind, sys::MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS);
    assert_eq!(raw.context, 0);
    assert_eq!(raw.canvas_selector_size, 14);

    let existing = WebGlContextDescriptor::existing(7).to_core();
    assert_eq!(existing.kind, sys::MLN_WEBGL_CONTEXT_EXISTING);
    assert_eq!(existing.context, 7);
    assert_eq!(existing.canvas_selector_size, 0);
}

#[test]
fn cpu_complete_sync_uses_native_default_shape() {
    let raw = GpuSync::CPU_COMPLETE.to_native();
    assert_eq!(raw.kind, sys::MLN_GPU_SYNC_CPU_COMPLETE);
    assert!(raw.object.is_null());
    assert_eq!(raw.value, 0);
    assert_eq!(raw.size as usize, std::mem::size_of::<sys::mln_gpu_sync>());
}

#[test]
fn render_session_control_is_send_and_sync() {
    fn assert_send_sync<T: Send + Sync>() {}
    assert_send_sync::<RenderSessionHandle>();
}
