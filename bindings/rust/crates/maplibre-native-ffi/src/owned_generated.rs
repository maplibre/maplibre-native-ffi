// Generated from C headers by tools/bindgen. Do not edit.
use super::*;
#[derive(Debug)]
pub struct BufferHandle {
    handle: crate::handle::ConcurrentNativeHandle<sys::mln_buffer>,
}
impl BufferHandle {
    pub(crate) fn from_native(raw: sys::mln_buffer) -> Result<Self> {
        Ok(Self {
            handle: unsafe {
                crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_buffer")
            }?,
        })
    }
    fn native(&self) -> Result<sys::mln_buffer> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("BufferHandle"))
    }
}
impl Drop for BufferHandle {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_buffer_destroy(raw) };
            Ok(())
        });
    }
}
#[derive(Debug)]
pub struct EventBatchHandle {
    handle: crate::handle::ConcurrentNativeHandle<sys::mln_event_batch>,
}
impl EventBatchHandle {
    pub(crate) fn from_native(raw: sys::mln_event_batch) -> Result<Self> {
        Ok(Self {
            handle: unsafe {
                crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_event_batch")
            }?,
        })
    }
    fn native(&self) -> Result<sys::mln_event_batch> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("EventBatchHandle"))
    }
}
impl Drop for EventBatchHandle {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_event_batch_release(raw) };
            Ok(())
        });
    }
}
#[derive(Debug)]
pub struct RenderFrameBatchHandle {
    handle: crate::handle::ConcurrentNativeHandle<sys::mln_render_frame_batch>,
}
impl RenderFrameBatchHandle {
    pub(crate) fn from_native(raw: sys::mln_render_frame_batch) -> Result<Self> {
        Ok(Self {
            handle: unsafe {
                crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_render_frame_batch")
            }?,
        })
    }
    fn native(&self) -> Result<sys::mln_render_frame_batch> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("RenderFrameBatchHandle"))
    }
}
impl Drop for RenderFrameBatchHandle {
    fn drop(&mut self) {
        self.handle.finalize_with(|raw| {
            unsafe { sys::mln_render_frame_batch_release(raw) };
            Ok(())
        });
    }
}
impl BufferHandle {
    /// Calls `mln_buffer_destroy` using its header execution and ownership contract.
    pub fn destroy(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.handle.close_with(|native| {
            unsafe { sys::mln_buffer_destroy(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_buffer_get` using its header execution and ownership contract.
    pub fn get(&self) -> Result<Vec<u8>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.handle.read_handle()?;
        let native = binding_read.native;
        maplibre_core::callback::check("mln_buffer_get", native.0)?;
        let mut binding_arg_1: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        maplibre_core::check(unsafe { sys::mln_buffer_get(native, &mut binding_arg_1) })?;
        Ok(unsafe { maplibre_core::string::copy_string_view_bytes(binding_arg_1) }?)
    }
}

impl EventBatchHandle {
    /// Calls `mln_event_batch_get` using its header execution and ownership contract.
    pub fn get(&self) -> Result<maplibre_core::generated::RuntimeEventBatchView> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.handle.read_handle()?;
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
        let result = self.handle.close_with(|native| {
            unsafe { sys::mln_event_batch_release(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}

impl RenderFrameBatchHandle {
    /// Calls `mln_render_frame_batch_count` using its header execution and ownership contract.
    pub fn count(&self) -> Result<usize> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.handle.read_handle()?;
        let native = binding_read.native;
        maplibre_core::callback::check("mln_render_frame_batch_count", native.0)?;
        let mut binding_arg_1: usize = Default::default();
        maplibre_core::check(unsafe {
            sys::mln_render_frame_batch_count(native, &mut binding_arg_1)
        })?;
        Ok(binding_arg_1)
    }

    /// Calls `mln_render_frame_batch_get` using its header execution and ownership contract.
    pub fn get(&self, binding_arg_1: usize) -> Result<maplibre_core::generated::RenderFrameResult> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_read = self.handle.read_handle()?;
        let native = binding_read.native;
        maplibre_core::callback::check("mln_render_frame_batch_get", native.0)?;
        let mut binding_arg_2: sys::mln_render_frame_result =
            maplibre_core::generated::RenderFrameResult::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_render_frame_batch_get(native, binding_arg_1, &mut binding_arg_2)
        })?;
        Ok(maplibre_core::generated::RenderFrameResult::from_native(
            binding_arg_2,
        ))
    }

    /// Calls `mln_render_frame_batch_release` using its header execution and ownership contract.
    pub fn release(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.handle.close_with(|native| {
            unsafe { sys::mln_render_frame_batch_release(native) };
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
