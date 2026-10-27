// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A rendered frame that a render session lends until its release.
    ///
    /// See `mln_acquired_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    #[must_use = "`AcquiredFrameHandle` must be released with `AcquiredFrameHandle::release`"]
    pub struct AcquiredFrameHandle(mln_acquired_frame) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_acquired_frame_discard(raw, out_diagnostic) });
    /// Returns the native handle value.
    fn id;
}

impl AcquiredFrameHandle {
    /// Consumes an acquired frame without consumer GPU synchronization, so it
    /// quarantines the frame's slot of the texture ring.
    ///
    /// See `mln_acquired_frame_discard` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn discard(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|frame| {
            let mut call = Call::new(frame);
            call.status(|frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_discard(frame, out_diagnostic)
            })
        })
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

    /// Releases an acquired frame after the host's GPU work that reads it.
    ///
    /// See `mln_acquired_frame_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(
        self,
        consumer_completion: Option<&GpuSync>,
    ) -> std::result::Result<(), (Self, Error)> {
        self.consume(|frame| {
            let mut call = Call::new(frame);
            let consumer_completion = call.optional_reference(consumer_completion.as_ref())?;
            call.status(|frame, out_diagnostic| unsafe {
                sys::mln_acquired_frame_release(frame, consumer_completion, out_diagnostic)
            })
        })
    }
}

/// A view scope of [`AcquiredFrameHandle`], which ends when it drops. Its
/// getters lend values that stay valid until then, and its owner can't
/// be consumed while it lives.
///
/// Begins an allocation-free scope that borrows an acquired frame.
///
/// See `mln_acquired_frame_view_begin` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub struct AcquiredFrameView<'a> {
    owner: &'a AcquiredFrameHandle,
    _scope: crate::call::ViewScope,
}
impl std::fmt::Debug for AcquiredFrameView<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "AcquiredFrameView(id={:#x})", self.owner.id())
    }
}
impl AcquiredFrameHandle {
    /// Begins an allocation-free scope that borrows an acquired frame.
    ///
    /// See `mln_acquired_frame_view_begin` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn view(&self) -> Result<AcquiredFrameView<'_>> {
        // SAFETY: these are this handle type's view scope functions.
        let scope = unsafe {
            crate::call::ViewScope::begin(
                self.inner.native(),
                sys::mln_acquired_frame_view_begin,
                sys::mln_acquired_frame_view_end,
            )
        }?;
        Ok(AcquiredFrameView {
            owner: self,
            _scope: scope,
        })
    }
}
impl AcquiredFrameView<'_> {
    /// Copies Metal-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_metal_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(any(
        target_os = "ios",
        target_os = "macos",
        target_os = "tvos",
        target_os = "visionos"
    ))]
    pub fn get_metal_texture(&self) -> Result<Lent<'_, MetalTextureFrame>> {
        let mut call = self.owner.inner.call();
        let mut out_frame: sys::mln_metal_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_metal_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_metal_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_frame) }?))
    }

    /// Copies OpenGL-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_open_gl_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    pub fn get_open_gl_texture(&self) -> Result<Lent<'_, OpenGlTextureFrame>> {
        let mut call = self.owner.inner.call();
        let mut out_frame: sys::mln_open_gl_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_open_gl_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_open_gl_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_frame) }?))
    }

    /// Copies the producer synchronization for an acquired texture frame.
    ///
    /// See `mln_acquired_frame_get_producer_sync` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_producer_sync(&self) -> Result<Lent<'_, GpuSync>> {
        let mut call = self.owner.inner.call();
        let mut out_sync: sys::mln_gpu_sync = unsafe { std::mem::zeroed() };
        out_sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_producer_sync(frame, &mut out_sync, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_sync) }?))
    }

    /// Copies Vulkan-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_vulkan_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub fn get_vulkan_texture(&self) -> Result<Lent<'_, VulkanTextureFrame>> {
        let mut call = self.owner.inner.call();
        let mut out_frame: sys::mln_vulkan_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_vulkan_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_frame) }?))
    }

    /// Copies WebGPU-native metadata from an acquired frame.
    ///
    /// See `mln_acquired_frame_get_web_gpu_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    #[cfg(target_os = "emscripten")]
    pub fn get_web_gpu_texture(&self) -> Result<Lent<'_, WebGpuTextureFrame>> {
        let mut call = self.owner.inner.call();
        let mut out_frame: sys::mln_web_gpu_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_web_gpu_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_web_gpu_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_frame) }?))
    }
}
