// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_render_session` native handle.
    ///
    /// A render session, which renders one map to one render target.
    ///
    /// See `mln_render_session` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RenderSessionHandle(mln_render_session) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_render_session_dispose(raw, out_diagnostic) });
}

impl RenderSessionHandle {
    /// Starts an ordered caller-owned Metal texture replacement.
    ///
    /// See `mln_metal_borrowed_texture_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn metal_borrowed_texture_set_target(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_metal_borrowed_texture_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_metal_borrowed_texture_set_target(
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
    /// See `mln_metal_surface_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn metal_surface_set_target(
        &self,
        descriptor: &MetalSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_metal_surface_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_metal_surface_set_target(session, descriptor, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Starts an ordered caller-owned OpenGL texture replacement.
    ///
    /// See `mln_opengl_borrowed_texture_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn opengl_borrowed_texture_set_target(
        &self,
        descriptor: &OpenglBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_opengl_borrowed_texture_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_opengl_borrowed_texture_set_target(
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
    /// See `mln_opengl_surface_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn opengl_surface_set_target(
        &self,
        descriptor: &OpenglSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_opengl_surface_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_opengl_surface_set_target(session, descriptor, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Irreversibly closes control and mailboxes and disposes of the session's
    /// graphics objects.
    ///
    /// See `mln_render_session_abandon` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn abandon(&self) -> Result<RenderAbandonResult> {
        let mut call = self.inner.call("mln_render_session_abandon")?;
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
        let mut call = self.inner.call("mln_render_session_acquire_frame")?;
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

    /// Starts a barrier that completes after all render work accepted before it
    /// has a terminal result. A barrier does not request a frame.
    ///
    /// See `mln_render_session_barrier` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_barrier")?;
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
        let call = self.inner.call("mln_render_session_clear_data")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_clear_data(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Retires a detached or abandoned session handle. The call is CPU-only and
    /// may run on any native thread, including from one of the session's own
    /// completions. If an abandonment is still in progress on another thread,
    /// this waits for it to finish before consuming the session owner.
    ///
    /// See `mln_render_session_destroy` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|session| {
            let mut call = Call::new(session, None);
            call.status(|session, out_diagnostic| unsafe {
                sys::mln_render_session_destroy(session, out_diagnostic)
            })
        })
    }

    /// Starts normal graphics-owner teardown and map detachment.
    ///
    /// See `mln_render_session_detach` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn detach(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_detach")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_detach(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Consumes a session and schedules its retirement and destruction.
    ///
    /// See `mln_render_session_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|session| {
            let mut call = Call::new(session, None);
            call.status(|session, out_diagnostic| unsafe {
                sys::mln_render_session_dispose(session, out_diagnostic)
            })
        })
    }

    /// Drains every currently queued terminal frame result into an
    /// independently owned batch. The records remain stable until the batch is
    /// released.
    ///
    /// See `mln_render_session_drain_frame_results` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn drain_frame_results(&self) -> Result<Option<RenderFrameBatchHandle>> {
        let mut call = self.inner.call("mln_render_session_drain_frame_results")?;
        let mut out_batch = sys::mln_render_frame_batch(0);
        if !call.status_unless(
            sys::MLN_STATUS_NOT_READY,
            |session, out_diagnostic| unsafe {
                sys::mln_render_session_drain_frame_results(session, &mut out_batch, out_diagnostic)
            },
        )? {
            return Ok(None);
        }
        Ok(Some(RenderFrameBatchHandle::adopt(out_batch, None)?))
    }

    /// Starts asynchronous renderer diagnostic-log emission.
    ///
    /// See `mln_render_session_dump_debug_logs` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_dump_debug_logs")?;
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
        let mut call = self.inner.call("mln_render_session_get_capabilities")?;
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
        let mut call = self.inner.call("mln_render_session_get_snapshot")?;
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_snapshot(session, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Copies the last completed rendered transform into an independent
    /// projection. Callable from any thread. Returns invalid state before a
    /// completed render, after an extent or target change, or after detachment.
    /// The caller owns the returned projection, which remains usable after the
    /// session is released. out_projection must point to a null handle.
    ///
    /// See `mln_render_session_projection_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn projection_create(&self) -> Result<MapProjectionHandle> {
        let mut call = self.inner.call("mln_render_session_projection_create")?;
        let mut out_projection = sys::mln_map_projection(0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_projection_create(session, &mut out_projection, out_diagnostic)
        })?;
        Ok(MapProjectionHandle::adopt(out_projection, None)?)
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
        feature: &[u8],
        extension: &str,
        extension_field: &str,
        arguments: Option<&[u8]>,
    ) -> Result<NativeFuture<Vec<u8>>> {
        let mut call = self
            .inner
            .call("mln_render_session_query_feature_extensions")?;
        let source_id = call.input(&source_id)?;
        let feature = call.input(&feature)?;
        let extension = call.input(&extension)?;
        let extension_field = call.input(&extension_field)?;
        let arguments = call.optional_reference(arguments)?;
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

    /// Starts a rendered-feature query against the session's latest driver
    /// state.
    ///
    /// See `mln_render_session_query_rendered_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_rendered_features(
        &self,
        geometry: &RenderedQueryGeometry,
        options: Option<&RenderedFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<QueriedFeature>>> {
        let mut call = self
            .inner
            .call("mln_render_session_query_rendered_features")?;
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
            completion::list::<sys::mln_queried_feature, _>,
        )
    }

    /// Starts a source-feature query against the session's latest driver state.
    /// The completion borrows an array of `mln_queried_feature` values
    /// (value_count entries), valid only for the callback.
    ///
    /// See `mln_render_session_query_source_features` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
    pub fn query_source_features(
        &self,
        source_id: &str,
        options: Option<&SourceFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<QueriedFeature>>> {
        let mut call = self
            .inner
            .call("mln_render_session_query_source_features")?;
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
            completion::list::<sys::mln_queried_feature, _>,
        )
    }

    /// Starts best-effort release of renderer caches.
    ///
    /// See `mln_render_session_reduce_memory_use` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn reduce_memory_use(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_reduce_memory_use")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_reduce_memory_use(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Requests a frame without waiting. Every accepted demand produces one
    /// terminal result record. A core worker wakes itself; a caller driver
    /// publishes its driver-work endpoint.
    ///
    /// See `mln_render_session_request_frame` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn request_frame(&self, demand: &FrameDemand) -> Result<()> {
        let mut call = self.inner.call("mln_render_session_request_frame")?;
        let demand = call.reference(&demand)?;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_request_frame(session, demand, out_diagnostic)
        })?;
        Ok(())
    }

    /// Starts an ordered logical resize. The completion runs after the selected
    /// driver applies the extent and updates the map viewport.
    ///
    /// See `mln_render_session_resize` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn resize(&self, extent: &RenderTargetExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_render_session_resize")?;
        let extent = call.reference(&extent)?;
        call.command(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, extent, completion, out_diagnostic)
        })
    }

    /// Services up to max_work items for a caller-graphics-thread driver; zero
    /// services every item currently queued. The first successful service call
    /// fixes the session's graphics-thread identity; later calls from another
    /// native thread return `MLN_STATUS_WRONG_THREAD`. The target context must
    /// be current. Core-worker sessions return `MLN_STATUS_INVALID_STATE`.
    ///
    /// See `mln_render_session_service_driver_work` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn service_driver_work(&self, max_work: usize) -> Result<usize> {
        let mut call = self.inner.call("mln_render_session_service_driver_work")?;
        let mut out_serviced: usize = Default::default();
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_service_driver_work(
                session,
                max_work,
                &mut out_serviced,
                out_diagnostic,
            )
        })?;
        Ok(out_serviced)
    }

    /// Starts readback of the latest rendered texture frame.
    ///
    /// See `mln_texture_read_premultiplied_rgba8` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    pub fn texture_read_premultiplied_rgba8(&self) -> Result<NativeFuture<TextureReadbackResult>> {
        let call = self.inner.call("mln_texture_read_premultiplied_rgba8")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_texture_read_premultiplied_rgba8(session, completion, out_diagnostic)
            },
            completion::value::<sys::mln_texture_readback_result, _>,
        )
    }

    /// Starts an ordered caller-owned Vulkan texture replacement.
    ///
    /// See `mln_vulkan_borrowed_texture_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn vulkan_borrowed_texture_set_target(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_vulkan_borrowed_texture_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_vulkan_borrowed_texture_set_target(
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
    /// See `mln_vulkan_surface_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn vulkan_surface_set_target(
        &self,
        descriptor: &VulkanSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_vulkan_surface_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_vulkan_surface_set_target(session, descriptor, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Starts an ordered caller-owned WebGPU texture replacement.
    ///
    /// See `mln_webgpu_borrowed_texture_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn webgpu_borrowed_texture_set_target(
        &self,
        descriptor: &WebgpuBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_webgpu_borrowed_texture_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_webgpu_borrowed_texture_set_target(
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
    /// See `mln_webgpu_surface_set_target` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
    ///
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    pub unsafe fn webgpu_surface_set_target(
        &self,
        descriptor: &WebgpuSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_webgpu_surface_set_target")?;
        let descriptor = call.reference(&descriptor)?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_webgpu_surface_set_target(session, descriptor, completion, out_diagnostic)
            },
            completion::unit,
        )
    }
}
