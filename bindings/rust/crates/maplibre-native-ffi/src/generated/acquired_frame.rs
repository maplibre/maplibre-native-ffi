// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_acquired_frame` native handle.
    ///
    /// A rendered frame that a render session lends until its release.
    ///
    /// See `mln_acquired_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    #[must_use = "`AcquiredFrameHandle` must be released with `AcquiredFrameHandle::release`"]
    pub struct AcquiredFrameHandle(mln_acquired_frame) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_acquired_frame_discard(raw, out_diagnostic) }), counted false;
    /// Returns the native handle value.
    fn id;
}

impl AcquiredFrameHandle {
    /// Consumes an acquired frame without consumer GPU synchronization, so it
    /// quarantines the frame's slot of the texture ring.
    ///
    /// See `mln_acquired_frame_discard` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn discard(&self) -> Result<()> {
        self.inner.close_once(|frame| {
            let mut call = Call::new(frame, None);
            call.status(|frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_discard(frame, out_diagnostic)
            })
        })
    }

    /// Copies Metal-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_metal_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(any(target_os = "ios", target_os = "macos", target_os = "tvos"))]
    pub fn get_metal_texture<R>(
        &self,
        callback: impl FnOnce(&MetalTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.call();
        unsafe {
            call.view(
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_metal_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_metal_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_metal_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Copies OpenGL-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_opengl_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    pub fn get_opengl_texture<R>(
        &self,
        callback: impl FnOnce(&OpenglTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.call();
        unsafe {
            call.view(
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_opengl_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_opengl_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_opengl_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Copies the producer synchronization for an acquired texture frame.
    ///
    /// See `mln_acquired_frame_get_producer_sync` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_producer_sync<R>(&self, callback: impl FnOnce(&GpuSync) -> R) -> Result<R> {
        let mut call = self.inner.call();
        unsafe {
            call.view(
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
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

    /// Copies common metadata for an acquired frame.
    ///
    /// See `mln_acquired_frame_get_result` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_result(&self) -> Result<RenderFrameResult> {
        let mut call = self.inner.call();
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_result(frame, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Copies Vulkan-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_vulkan_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(any(
        target_os = "android",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub fn get_vulkan_texture<R>(
        &self,
        callback: impl FnOnce(&VulkanTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.call();
        unsafe {
            call.view(
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_vulkan_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_vulkan_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Copies WebGPU-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_webgpu_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(target_os = "emscripten")]
    pub fn get_webgpu_texture<R>(
        &self,
        callback: impl FnOnce(&WebgpuTextureFrame) -> R,
    ) -> Result<R> {
        let mut call = self.inner.call();
        unsafe {
            call.view(
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
            )
        }?;
        let mut out_frame: sys::mln_webgpu_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_webgpu_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_webgpu_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        let value = unsafe { from_native(out_frame) }?;
        Ok(callback(&value))
    }

    /// Releases an acquired frame after the host's GPU work that reads it.
    ///
    /// See `mln_acquired_frame_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(&self, consumer_completion: Option<&GpuSync>) -> Result<()> {
        self.inner.close_once(|frame| {
            let mut call = Call::new(frame, None);
            let consumer_completion = call.optional_reference(consumer_completion.as_ref())?;
            call.status(|frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_release(frame, consumer_completion, out_diagnostic)
            })
        })
    }
}
