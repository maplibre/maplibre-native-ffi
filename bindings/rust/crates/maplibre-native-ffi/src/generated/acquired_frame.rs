// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A rendered frame that the caller holds to read or show.
    ///
    /// See `mln_acquired_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    #[must_use = "`AcquiredFrameHandle` must be released with `AcquiredFrameHandle::release`"]
    pub struct AcquiredFrameHandle(mln_acquired_frame) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_acquired_frame_discard(raw, out_diagnostic) });
    /// Returns the native handle value.
    fn id;
}

impl AcquiredFrameHandle {
    /// Gives up an acquired frame without saying when your GPU stops reading
    /// it.
    ///
    /// Because your GPU may still read the frame's texture, the session never
    /// renders into that texture again.
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

    /// Reads the frame result of an acquired frame.
    ///
    /// It succeeds until you release or discard the frame, whatever the
    /// session's state.
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

    /// Gives an acquired frame back to the session once you're done reading it.
    ///
    /// `consumer_completion` says when your GPU work that reads the texture
    /// finishes. Pass `GpuSyncKind::CpuComplete` when that work has already
    /// finished. The session waits for that point before it renders into the
    /// texture again. If that wait fails, such as for an OpenGL fence outside
    /// the session's share group, the session fails and never renders into the
    /// texture again.
    ///
    /// See `mln_acquired_frame_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when
    ///   `RenderSessionCapabilities::consumer_sync_kinds` lacks the kind of
    ///   `consumer_completion`.
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn release(
        self,
        consumer_completion: &GpuSync,
    ) -> std::result::Result<(), (Self, Error)> {
        self.consume(|frame| {
            let mut call = Call::new(frame);
            let consumer_completion = call.reference(&consumer_completion)?;
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
/// Keeps an acquired frame's texture and sync objects in place while you use
/// them.
///
/// See `mln_acquired_frame_view_begin` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
///
/// # Errors
///
/// - `Status::TargetLost` when the session failed, or you detached, abandoned,
///   or disposed it.
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
    /// Keeps an acquired frame's texture and sync objects in place while you
    /// use them.
    ///
    /// See `mln_acquired_frame_view_begin` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
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
    /// Gets the Metal texture of an acquired frame.
    ///
    /// See `mln_acquired_frame_get_metal_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
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

    /// Gets the OpenGL texture of an acquired frame.
    ///
    /// See `mln_acquired_frame_get_open_gl_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
    pub fn get_open_gl_texture(&self) -> Result<Lent<'_, OpenGlTextureFrame>> {
        let mut call = self.owner.inner.call();
        let mut out_frame: sys::mln_open_gl_texture_frame = unsafe { std::mem::zeroed() };
        out_frame.size = std::mem::size_of::<sys::mln_open_gl_texture_frame>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_open_gl_texture(frame, &mut out_frame, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_frame) }?))
    }

    /// Reads what to wait for before you read an acquired frame's texture.
    ///
    /// See `mln_acquired_frame_get_producer_sync` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
    pub fn get_producer_sync(&self) -> Result<Lent<'_, GpuSync>> {
        let mut call = self.owner.inner.call();
        let mut out_sync: sys::mln_gpu_sync = unsafe { std::mem::zeroed() };
        out_sync.size = std::mem::size_of::<sys::mln_gpu_sync>() as _;
        call.status(|frame, out_diagnostic| unsafe {
            sys::mln_acquired_frame_get_producer_sync(frame, &mut out_sync, out_diagnostic)
        })?;
        Ok(Lent::new(unsafe { from_native(out_sync) }?))
    }

    /// Gets the Vulkan image of an acquired frame.
    ///
    /// See `mln_acquired_frame_get_vulkan_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
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

    /// Gets the WebGPU texture of an acquired frame.
    ///
    /// See `mln_acquired_frame_get_web_gpu_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::TargetLost` when the session failed, or you detached,
    ///   abandoned, or disposed it.
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
