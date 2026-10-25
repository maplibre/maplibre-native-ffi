// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_render_session` native handle.
    ///
    /// A render session, which renders one map to one render target.
    ///
    /// See `mln_render_session` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RenderSessionHandle(mln_render_session) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_render_session_dispose(raw, std::ptr::null(), out_diagnostic) });
    /// Returns the native handle value.
    fn id;
}

impl RenderSessionHandle {
    /// Irreversibly ends session work and destroys the session's graphics
    /// objects where that is safe.
    ///
    /// See `mln_render_session_abandon` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn abandon(&self) -> Result<RenderAbandonResult> {
        let mut call = self.inner.call();
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_abandon(session, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Acquires the oldest rendered frame that is not already acquired. The
    /// frame owns its slot until release. The call is nonblocking.
    ///
    /// See `mln_render_session_acquire_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn acquire_frame(&self) -> Result<Option<AcquiredFrameHandle>> {
        let mut call = self.inner.call();
        let mut out_frame = sys::mln_acquired_frame(0);
        if !call.status_unless(
            sys::MLN_STATUS_NOT_READY,
            |session, out_diagnostic| unsafe {
                sys::mln_render_session_acquire_frame(session, &mut out_frame, out_diagnostic)
            },
        )? {
            return Ok(None);
        }
        let parent = self.inner.parent();
        Ok(Some(AcquiredFrameHandle::adopt(out_frame, parent)?))
    }

    /// Ends every frame demand that has not started with
    /// `MLN_RENDER_RESULT_NO_UPDATE` and completes after earlier driver work
    /// has finished. A running demand finishes its attempt and ends any wait
    /// for a map update or a free slot. After completion, a new frame demand is
    /// required to render again, except that a pending still image submits its
    /// next demand itself; see `mln_map_request_still_image()`.
    ///
    /// See `mln_render_session_barrier` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_barrier(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Starts asynchronous renderer-data clearing.
    ///
    /// See `mln_render_session_clear_data` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn clear_data(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_clear_data(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Starts normal graphics-owner teardown and map detachment.
    ///
    /// See `mln_render_session_detach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn detach(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_detach(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Consumes a session in any state and retires it, then runs completion, if
    /// given, once the host may destroy the target's graphics objects.
    ///
    /// See `mln_render_session_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(&self) -> Result<NativeFuture<()>> {
        self.inner.close(|session| {
            let call = Call::new(session);
            call.complete(
                |session, completion, out_diagnostic| unsafe {
                    sys::mln_render_session_dispose(session, completion, out_diagnostic)
                },
                completion::unit,
            )
        })
    }

    /// Consumes a session in any state and retires it, then runs completion, if
    /// given, once the host may destroy the target's graphics objects.
    ///
    /// See `mln_render_session_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn dispose(&self) -> Result<()> {
        self.inner.close_once(|session| {
            let call = Call::new(session);
            call.discard(|session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_dispose(session, completion, out_diagnostic)
            })
        })
    }

    /// Drains every currently queued terminal frame result into an
    /// independently owned batch and borrows its view. The records remain
    /// stable until the batch is released.
    ///
    /// See `mln_render_session_drain_frame_results` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn drain_frame_results(&self) -> Result<Vec<RenderFrameResult>> {
        let mut call = self.inner.call();
        let mut out_batch = sys::mln_render_frame_batch(0);
        let mut out_view: sys::mln_render_frame_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_render_frame_batch_view>() as _;
        if !call.status_unless(
            sys::MLN_STATUS_NOT_READY,
            |session, out_diagnostic| unsafe {
                sys::mln_render_session_drain_frame_results(
                    session,
                    &mut out_batch,
                    &mut out_view,
                    out_diagnostic,
                )
            },
        )? {
            return Ok(Vec::new());
        }
        // SAFETY: native published the batch to this call alone.
        let _batch =
            unsafe { crate::handle::Drained::new(out_batch, sys::mln_render_frame_batch_release) };
        Ok(unsafe {
            convert::copy_strided(
                out_view.results,
                out_view.result_count,
                out_view.result_size,
            )
        }?)
    }

    /// Starts asynchronous renderer diagnostic-log emission.
    ///
    /// See `mln_render_session_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_dump_debug_logs(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Returns the immutable capabilities fixed during attachment.
    ///
    /// See `mln_render_session_get_capabilities` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_capabilities(&self) -> Result<RenderSessionCapabilities> {
        let mut call = self.inner.call();
        let mut out_capabilities: sys::mln_render_session_capabilities =
            unsafe { std::mem::zeroed() };
        out_capabilities.size = std::mem::size_of::<sys::mln_render_session_capabilities>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_capabilities(session, &mut out_capabilities, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_capabilities) }?)
    }

    /// Copies the latest render-session snapshot from any native thread.
    ///
    /// See `mln_render_session_get_snapshot` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get_snapshot(&self) -> Result<RenderSessionSnapshot> {
        let mut call = self.inner.call();
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_snapshot(session, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Starts a feature-extension query against the latest driver state. The
    /// completion borrows one `mln_buffer_view` holding UTF-8 JSON (value_count
    /// 1), valid only for the callback.
    ///
    /// See `mln_render_session_query_feature_extensions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_feature_extensions(
        &self,
        source_id: &str,
        feature: &str,
        extension: &str,
        extension_field: &str,
        arguments: &str,
    ) -> Result<NativeFuture<String>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let feature = call.input(&feature)?;
        let extension = call.input(&extension)?;
        let extension_field = call.input(&extension_field)?;
        let arguments = call.input(&arguments)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_feature_extensions(
                    session,
                    source_id,
                    feature,
                    extension,
                    extension_field,
                    arguments,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_buffer_view, _>,
        )
    }

    /// Starts a rendered-feature query against the last frame that the session
    /// rendered, so render after a camera or style change for a result that
    /// matches it.
    ///
    /// See `mln_render_session_query_rendered_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<RenderedFeature>>> {
        let mut call = self.inner.call();
        let geometry = call.reference(&geometry)?;
        let options = call.optional_reference(options.as_ref())?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_rendered_features(
                    session,
                    geometry,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_rendered_feature, _>,
        )
    }

    /// Starts a source-feature query against the tiles that the source loaded
    /// for rendering, which it loads only while a visible layer uses it.
    ///
    /// See `mln_render_session_query_source_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<SourceFeature>>> {
        let mut call = self.inner.call();
        let source_id = call.input(&source_id)?;
        let options = call.optional_reference(options.as_ref())?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_query_source_features(
                    session,
                    source_id,
                    options,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_source_feature, _>,
        )
    }

    /// Reads back the latest frame of the session's owned texture as
    /// premultiplied RGBA8.
    ///
    /// See `mln_render_session_read_texture` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    pub fn read_texture(&self) -> Result<NativeFuture<TextureReadbackResult>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_read_texture(session, completion, out_diagnostic)
            },
            completion::value::<sys::mln_texture_readback_result, _>,
        )
    }

    /// Starts best-effort release of renderer caches.
    ///
    /// See `mln_render_session_reduce_memory_use` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn reduce_memory_use(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_reduce_memory_use(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Requests a frame without waiting. Every accepted demand produces one
    /// terminal result record. Caller-driver sessions require service on the
    /// graphics thread. Core-worker sessions need no host service.
    ///
    /// See `mln_render_session_request_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn request_frame(&self, demand: &FrameDemand) -> Result<()> {
        let mut call = self.inner.call();
        let demand = call.reference(&demand)?;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_request_frame(session, demand, out_diagnostic)
        })?;
        Ok(())
    }

    /// Starts an ordered logical resize of the driver and the map. The driver
    /// applies the extent in order with its other work, and later driver work
    /// does not wait for the map. The completion runs after both have applied
    /// it and the map has published its snapshot. Frames render once a map
    /// update matches the applied extent; static and tile maps publish that
    /// update for their next still image.
    ///
    /// See `mln_render_session_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn resize(&self, extent: LogicalExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let extent = call.input(&extent)?;
        call.command(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, extent, completion, out_diagnostic)
        })
    }

    /// Services every queued item for a caller-graphics-thread driver. The
    /// first successful service call fixes the session's graphics-thread
    /// identity; later calls from another native thread return
    /// `MLN_STATUS_WRONG_THREAD`. The target context must be current.
    /// Core-worker sessions return `MLN_STATUS_INVALID_STATE`.
    ///
    /// See `mln_render_session_service_driver_work` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn service_driver_work(&self) -> Result<()> {
        let mut call = self.inner.call();
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_service_driver_work(session, out_diagnostic)
        })?;
        Ok(())
    }

    /// Starts an ordered replacement of every texture of a caller-owned Metal
    /// ring.
    ///
    /// See `mln_render_session_set_metal_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(target_os = "ios", target_os = "macos", target_os = "tvos"))]
    pub unsafe fn set_metal_borrowed_texture_target(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_metal_borrowed_texture_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered Metal surface replacement.
    ///
    /// See `mln_render_session_set_metal_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(target_os = "ios", target_os = "macos", target_os = "tvos"))]
    pub unsafe fn set_metal_surface_target(
        &self,
        descriptor: &MetalSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_metal_surface_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered replacement of every texture of a caller-owned OpenGL
    /// ring.
    ///
    /// See `mln_render_session_set_opengl_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn set_opengl_borrowed_texture_target(
        &self,
        descriptor: &OpenglBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_opengl_borrowed_texture_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered OpenGL surface replacement.
    ///
    /// See `mln_render_session_set_opengl_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn set_opengl_surface_target(
        &self,
        descriptor: &OpenglSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_opengl_surface_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered replacement of every image of a caller-owned Vulkan
    /// ring.
    ///
    /// See `mln_render_session_set_vulkan_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn set_vulkan_borrowed_texture_target(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_vulkan_borrowed_texture_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered Vulkan surface replacement.
    ///
    /// See `mln_render_session_set_vulkan_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(any(
        target_os = "android",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub unsafe fn set_vulkan_surface_target(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_vulkan_surface_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered replacement of every texture of a caller-owned WebGPU
    /// ring.
    ///
    /// See `mln_render_session_set_webgpu_borrowed_texture_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn set_webgpu_borrowed_texture_target(
        &self,
        descriptor: &WebgpuBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_webgpu_borrowed_texture_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts an ordered WebGPU surface replacement.
    ///
    /// See `mln_render_session_set_webgpu_surface_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    #[cfg(target_os = "emscripten")]
    pub unsafe fn set_webgpu_surface_target(
        &self,
        descriptor: &WebgpuSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_set_webgpu_surface_target(
                    session,
                    descriptor,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }
}

/// Submits [`RenderSessionHandle`] commands without a completion. Each call returns
/// the same acceptance status as its completing form, and native logs a
/// command that later fails as a warning through the log handler.
pub struct RenderSessionHandleDiscarding<'a>(&'a RenderSessionHandle);
impl RenderSessionHandle {
    pub fn discarding(&self) -> RenderSessionHandleDiscarding<'_> {
        RenderSessionHandleDiscarding(self)
    }
}
impl RenderSessionHandleDiscarding<'_> {
    /// Starts an ordered logical resize of the driver and the map. The driver
    /// applies the extent in order with its other work, and later driver work
    /// does not wait for the map. The completion runs after both have applied
    /// it and the map has published its snapshot. Frames render once a map
    /// update matches the applied extent; static and tile maps publish that
    /// update for their next still image.
    ///
    /// See `mln_render_session_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn resize(&self, extent: LogicalExtent) -> Result<()> {
        let mut call = self.0.inner.call();
        let extent = call.input(&extent)?;
        call.discard(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, extent, completion, out_diagnostic)
        })
    }
}
