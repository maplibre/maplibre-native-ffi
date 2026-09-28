// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct EventBatchHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_event_batch>,
    id: u64,
}
impl EventBatchHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_event_batch> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("EventBatchHandle"))
    }
}
impl Drop for EventBatchHandleState {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_event_batch_release(raw) };
            Ok(())
        });
    }
}
/// Owns one `mln_event_batch` native handle.
pub struct EventBatchHandle {
    pub(crate) inner: std::sync::Arc<EventBatchHandleState>,
}
impl std::fmt::Debug for EventBatchHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("EventBatchHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl EventBatchHandle {
    pub(crate) fn from_native(raw: sys::mln_event_batch) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle =
            unsafe { crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_event_batch") }?;
        Ok(Self {
            inner: std::sync::Arc::new(EventBatchHandleState { handle, id: raw.0 }),
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

impl EventBatchHandle {
    /// Calls `mln_event_batch_get` using its header execution and ownership contract.
    pub fn get(&self) -> Result<maplibre_core::generated::RuntimeEventBatchView> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.inner.handle.read_handle()?;
        let native = binding_read.native;
        maplibre_core::callback::check("mln_event_batch_get", native.0)?;
        let mut binding_arg_1: sys::mln_runtime_event_batch_view = {
            let mut value: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
            value.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as _;
            value
        };
        maplibre_core::check(unsafe { sys::mln_event_batch_get(native, &mut binding_arg_1) })?;
        Ok(unsafe { maplibre_core::generated::RuntimeEventBatchView::from_native(binding_arg_1) }?)
    }

    /// Calls `mln_event_batch_release` using its header execution and ownership contract.
    pub fn release(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            unsafe { sys::mln_event_batch_release(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
