// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_acquired_frame` native handle.
    #[must_use = "`AcquiredFrameHandle` must be released with `AcquiredFrameHandle::release`"]
    pub struct AcquiredFrameHandle(mln_acquired_frame) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_acquired_frame_dispose(raw, out_diagnostic) });
}

impl AcquiredFrameHandle {
    /// Calls `mln_acquired_frame_dispose`.
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|frame| {
            let mut call = Call::new(frame, None);
            call.status(|frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_dispose(frame, out_diagnostic)
            })
        })
    }

    /// Calls `mln_acquired_frame_get_metal_texture`.
    pub fn get_metal_texture<R>(
        &self,
        callback: impl FnOnce(&MetalOwnedTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.read("mln_acquired_frame_get_metal_texture")?;
        unsafe {
            call.view(
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_metal_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_metal_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_opengl_texture`.
    pub fn get_opengl_texture<R>(
        &self,
        callback: impl FnOnce(&OpenglOwnedTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.read("mln_acquired_frame_get_opengl_texture")?;
        unsafe {
            call.view(
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_opengl_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_opengl_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_producer_sync`.
    pub fn get_producer_sync<R>(&self, callback: impl FnOnce(&GpuSync) -> R) -> Result<R> {
        let mut call = self.inner.read("mln_acquired_frame_get_producer_sync")?;
        unsafe {
            call.view(
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut out_sync: sys::mln_gpu_sync = unsafe { sys::mln_gpu_sync_default() };
        out_sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_producer_sync(frame, &mut out_sync, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_sync) }?;
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_result`.
    pub fn get_result(&self) -> Result<RenderFrameResult> {
        let mut call = self.inner.call("mln_acquired_frame_get_result")?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_result(frame, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Calls `mln_acquired_frame_get_vulkan_texture`.
    pub fn get_vulkan_texture<R>(
        &self,
        callback: impl FnOnce(&VulkanOwnedTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.read("mln_acquired_frame_get_vulkan_texture")?;
        unsafe {
            call.view(
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_vulkan_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_webgpu_texture`.
    pub fn get_webgpu_texture<R>(
        &self,
        callback: impl FnOnce(&WebgpuOwnedTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.read("mln_acquired_frame_get_webgpu_texture")?;
        unsafe {
            call.view(
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_webgpu_owned_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_webgpu_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_release`.
    pub fn release(&self, consumer_completion: &GpuSync) -> Result<()> {
        self.inner.close(|frame| {
            let mut call = Call::new(frame, None);
            let consumer_completion = call.reference(&consumer_completion)?;
            call.status(|mut frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_release(&mut frame, consumer_completion, out_diagnostic)
            })
        })
    }
}
