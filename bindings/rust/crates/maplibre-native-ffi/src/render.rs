#[allow(clippy::all, unused_parens)]
mod frame_generated;
#[allow(clippy::all, unused_parens)]
mod generated;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::map::MapState;
use crate::{ErrorKind, NativeFuture, Result};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;
use std::fmt;
use std::sync::Arc;

#[derive(Debug)]
pub(crate) struct RenderSessionState {
    handle: ConcurrentNativeHandle<sys::mln_render_session>,
    /// Keeps the session's parent map alive. Native holds the map while a
    /// session is attached, so the binding holds the same reference and the
    /// host may drop its map handle before it destroys the session.
    _map: Arc<MapState>,
}

impl RenderSessionState {
    fn new(native: sys::mln_render_session, map: Arc<MapState>) -> Result<Self> {
        // SAFETY: native came from an accepted attach submission, and session
        // control state is safe to inspect from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_render_session") }?;
        Ok(Self { handle, _map: map })
    }

    fn native(&self) -> Result<sys::mln_render_session> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RenderSessionHandle"))
    }
}

impl Drop for RenderSessionState {
    fn drop(&mut self) {
        self.handle.finalize_with(|handle| unsafe {
            maplibre_core::generated::render_session_dispose(handle)
        });
    }
}

/// Send-safe render-session control handle.
#[derive(Clone)]
pub struct RenderSessionHandle {
    inner: Arc<RenderSessionState>,
}

impl fmt::Debug for RenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RenderSessionHandle")
            .field("closed", &self.inner.handle.is_closed())
            .finish()
    }
}

impl RenderSessionHandle {
    pub(crate) fn from_native(
        native: sys::mln_render_session,
        parent: Arc<MapState>,
    ) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(RenderSessionState::new(native, parent)?),
        })
    }
}
/// Owned lease for one session-owned texture-ring slot.
///
/// Call [`AcquiredFrameHandle::release`] with the completion of every consumer
/// GPU access. Dropping a frame quarantines its session and retires the lease
/// after in-flight driver calls return.
#[must_use = "an acquired frame must be released with the consumer's GPU completion"]
pub struct AcquiredFrameHandle {
    handle: ConcurrentNativeHandle<sys::mln_acquired_frame>,
    _parent: Arc<RenderSessionState>,
}

impl AcquiredFrameHandle {
    pub(crate) fn from_native(
        raw: sys::mln_acquired_frame,
        parent: Arc<RenderSessionState>,
    ) -> Result<Self> {
        if raw.0 == 0 {
            return Err(crate::Error::new(
                ErrorKind::NativeError,
                None,
                "frame acquisition returned a null frame",
            ));
        }
        Ok(Self {
            handle: unsafe { ConcurrentNativeHandle::from_handle(raw, "mln_acquired_frame") }?,
            _parent: parent,
        })
    }

    fn native(&self) -> Result<sys::mln_acquired_frame> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("AcquiredFrameHandle"))
    }
}
impl Drop for AcquiredFrameHandle {
    fn drop(&mut self) {
        self.handle.finalize_with(|frame| unsafe {
            maplibre_core::generated::acquired_frame_dispose(frame)
        });
    }
}

#[cfg(test)]
mod tests;
