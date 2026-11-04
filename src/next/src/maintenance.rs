// Ports maplibre-native-ffi render_session_maintenance_start and
// render_session_release_target_start (src/render/render_session_common.cpp,
// src/c_api/render_session.cpp): mln_render_session_reduce_memory_use,
// mln_render_session_clear_data, mln_render_session_dump_debug_logs and
// mln_render_session_release_target, for the offscreen and Metal surface
// sessions.
//
// Deviation: the core's Renderer::dumpDebugLogs is not ported, so
// mln_render_session_dump_debug_logs logs the session's own state instead of
// the renderer's sources and tiles.

use crate::delivery::{Completion, Value};
use crate::diagnostics::{Failure, boundary};
use crate::handles;
use crate::query::{submit, with_session_renderer};
use crate::types::*;
use maplibre_native_next::mln_gfx::{BackendScope, ScopeType};
use maplibre_native_next::mln_util::logging::{Event, Log};
use std::sync::atomic::Ordering;

#[derive(Clone, Copy)]
enum Maintenance {
    ReduceMemoryUse,
    ClearData,
    DumpDebugLogs,
}

/// `render_session_maintenance_start`.
///
/// # Safety
/// `completion` as for `Completion::required`.
unsafe fn maintain(
    session: mln_render_session,
    completion: *const mln_completion,
    kind: Maintenance,
) -> Result<(), Failure> {
    // SAFETY: the caller's contract.
    unsafe {
        submit(session, completion, move |session, map, shared| {
            with_session_renderer(session, |renderer, backend| {
                let _scope = BackendScope::new(backend, ScopeType::Explicit);
                match kind {
                    Maintenance::ReduceMemoryUse => renderer.reduce_memory_use(),
                    Maintenance::ClearData => renderer.clear_data(),
                    Maintenance::DumpDebugLogs => {
                        let public = shared.state.lock().unwrap();
                        Log::info(
                            Event::General,
                            format!(
                                "render session {session:#x}: state {}, extent {}x{} at {}, frame {}, map update {}",
                                public.state,
                                public.extent.width,
                                public.extent.height,
                                public.extent.scale_factor,
                                shared.frame_generation.load(Ordering::SeqCst),
                                map.frontend.update_generation(),
                            ),
                        );
                        Log::info(
                            Event::General,
                            "--------------------------------------------------------------------------------",
                        );
                    }
                }
            })
            .ok_or_else(|| Failure::invalid_state("render session is detached"))?;
            if let Maintenance::ClearData = kind {
                // The renderer lost its sources and their state, and draws
                // the map again (C++ reset_pushed_feature_state and
                // map_post_trigger_repaint).
                if let Some(entry) = crate::session::session_entry(session) {
                    *entry.feature_state.borrow_mut() = Default::default();
                }
                crate::metal_surface::reset_feature_state(session);
                map.map.borrow_mut().trigger_repaint();
            }
            Ok(Value::None)
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_reduce_memory_use(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            maintain(session, completion, Maintenance::ReduceMemoryUse)
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_clear_data(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            maintain(session, completion, Maintenance::ClearData)
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_dump_debug_logs(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            maintain(session, completion, Maintenance::DumpDebugLogs)
        })
    }
}

/// # Safety
/// C API contract (render_session.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_render_session_release_target(
    session: mln_render_session,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> i32 {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let completion = Completion::required(completion)?;
            let shared = handles::session(session)?;
            if shared.state() != MLN_RENDER_SESSION_STATE_ATTACHED {
                return Err(Failure::invalid_state("render session is not attached"));
            }
            if shared.metal.is_some() {
                return crate::metal_surface::release_target(session.0, completion);
            }
            if shared.driver == MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "a WebGPU surface session can't release its surface yet",
                ));
            }
            Err(Failure::new(
                MLN_STATUS_UNSUPPORTED,
                "an offscreen session has no target for the host to release",
            ))
        })
    }
}
