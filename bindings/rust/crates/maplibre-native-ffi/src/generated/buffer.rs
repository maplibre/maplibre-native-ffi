// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct BufferHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_buffer>,
    id: u64,
}
impl BufferHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_buffer> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("BufferHandle"))
    }
}
impl Drop for BufferHandleState {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_buffer_destroy(raw) };
            Ok(())
        });
    }
}
/// Owns one `mln_buffer` native handle.
pub struct BufferHandle {
    pub(crate) inner: std::sync::Arc<BufferHandleState>,
}
impl std::fmt::Debug for BufferHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("BufferHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl BufferHandle {
    pub(crate) fn from_native(raw: sys::mln_buffer) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle =
            unsafe { crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_buffer") }?;
        Ok(Self {
            inner: std::sync::Arc::new(BufferHandleState { handle, id: raw.0 }),
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

impl BufferHandle {
    /// Calls `mln_buffer_destroy` using its header execution and ownership contract.
    pub fn destroy(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            unsafe { sys::mln_buffer_destroy(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_buffer_get` using its header execution and ownership contract.
    pub fn get(&self) -> Result<Vec<u8>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.inner.handle.read_handle()?;
        let native = binding_read.native;
        maplibre_core::callback::check("mln_buffer_get", native.0)?;
        let mut binding_arg_1: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_buffer_get(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(binding_arg_1) }?)
    }
}
