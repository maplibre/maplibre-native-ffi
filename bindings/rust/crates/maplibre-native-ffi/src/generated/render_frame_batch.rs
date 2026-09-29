// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct RenderFrameBatchHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_render_frame_batch>,
    id: u64,
}
impl RenderFrameBatchHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_render_frame_batch> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("RenderFrameBatchHandle"))
    }
}
impl Drop for RenderFrameBatchHandleState {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_render_frame_batch_release(raw) };
            Ok(())
        });
    }
}
/// Owns one `mln_render_frame_batch` native handle.
pub struct RenderFrameBatchHandle {
    pub(crate) inner: std::sync::Arc<RenderFrameBatchHandleState>,
}
impl std::fmt::Debug for RenderFrameBatchHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RenderFrameBatchHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl RenderFrameBatchHandle {
    pub(crate) fn from_native(raw: sys::mln_render_frame_batch) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle = unsafe {
            crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_render_frame_batch")
        }?;
        Ok(Self {
            inner: std::sync::Arc::new(RenderFrameBatchHandleState { handle, id: raw.0 }),
        })
    }

    /// Returns the native handle value, which event sources report for this handle.
    pub fn id(&self) -> u64 {
        self.inner.id
    }

    /// Reports whether an explicit release, close, or disposal consumed this handle.
    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
    }
}

impl RenderFrameBatchHandle {
    /// Calls `mln_render_frame_batch_count` using its header execution and ownership contract.
    pub fn count(&self) -> Result<usize> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_frame_batch_count", native.0)?;
        let mut binding_arg_1: usize = Default::default();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_frame_batch_count(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(binding_arg_1)
    }

    /// Calls `mln_render_frame_batch_get` using its header execution and ownership contract.
    pub fn get(&self, binding_arg_1: usize) -> Result<maplibre_core::generated::RenderFrameResult> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_frame_batch_get", native.0)?;
        let mut binding_arg_2: sys::mln_render_frame_result =
            maplibre_core::generated::RenderFrameResult::default().to_native();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_frame_batch_get(native, binding_arg_1, &mut binding_arg_2, diagnostic)
        })?;
        Ok(maplibre_core::generated::RenderFrameResult::from_native(
            binding_arg_2,
        ))
    }

    /// Calls `mln_render_frame_batch_release` using its header execution and ownership contract.
    pub fn release(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            unsafe { sys::mln_render_frame_batch_release(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
