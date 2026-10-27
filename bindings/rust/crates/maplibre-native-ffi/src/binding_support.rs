//! Safe forms of the `binding_support.h` exports that a host runtime's
//! shutdown needs.

use maplibre_native_ffi_sys as sys;

use crate::convert::from_native;
use crate::{RenderAbandonResult, RenderSessionHandle, Result};

impl RenderSessionHandle {
    /// Abandons the session during a host runtime's shutdown, keeping its
    /// graphics objects.
    ///
    /// Follows [`RenderSessionHandle::abandon`], including its wait for an
    /// in-flight core-worker driver call, but makes no graphics call and
    /// keeps every graphics object until the process exits. The host may call
    /// this after its runtime's shutdown has destroyed the graphics objects
    /// behind the session, and still disposes the session afterwards.
    ///
    /// See `mln_binding_render_session_abandon_at_exit` in
    /// `maplibre_native_c/binding_support.h`.
    pub fn abandon_at_exit(&self) -> Result<RenderAbandonResult> {
        let mut call = self.inner.call();
        // SAFETY: all zeroes is a valid result, whose size native reads.
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        call.status(|session, out_diagnostic| {
            // SAFETY: out_result is a writable, correctly sized output.
            unsafe {
                sys::mln_binding_render_session_abandon_at_exit(
                    session,
                    &mut out_result,
                    out_diagnostic,
                )
            }
        })?;
        // SAFETY: the result holds no pointers.
        unsafe { from_native(out_result) }
    }
}
