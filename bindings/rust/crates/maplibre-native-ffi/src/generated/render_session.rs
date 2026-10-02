// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_render_session` native handle.
    pub struct RenderSessionHandle(mln_render_session) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_render_session_dispose(raw, out_diagnostic) });
}

impl RenderSessionHandle {
    /// Calls `mln_metal_borrowed_texture_set_target`.
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

    /// Calls `mln_metal_surface_set_target`.
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

    /// Calls `mln_opengl_borrowed_texture_set_target`.
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

    /// Calls `mln_opengl_surface_set_target`.
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

    /// Calls `mln_render_session_abandon`.
    pub fn abandon(&self) -> Result<RenderAbandonResult> {
        let mut call = self.inner.call("mln_render_session_abandon")?;
        let mut out_result: sys::mln_render_abandon_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_abandon_result>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_abandon(session, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Calls `mln_render_session_acquire_frame`.
    pub fn acquire_frame(&self) -> Result<AcquiredFrameHandle> {
        let mut call = self.inner.call("mln_render_session_acquire_frame")?;
        let parent = self.inner.parent();
        let mut out_frame = sys::mln_acquired_frame(0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_acquire_frame(session, &mut out_frame, out_diagnostic)
        })?;
        Ok(AcquiredFrameHandle::adopt(out_frame, parent)?)
    }

    /// Calls `mln_render_session_barrier`.
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_barrier")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_barrier(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_render_session_clear_data`.
    pub fn clear_data(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_clear_data")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_clear_data(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_render_session_destroy`.
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|session| {
            let mut call = Call::new(session, None);
            call.status(|session, out_diagnostic| unsafe {
                sys::mln_render_session_destroy(session, out_diagnostic)
            })
        })
    }

    /// Calls `mln_render_session_detach`.
    pub fn detach(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_detach")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_detach(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_render_session_dispose`.
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|session| {
            let mut call = Call::new(session, None);
            call.status(|session, out_diagnostic| unsafe {
                sys::mln_render_session_dispose(session, out_diagnostic)
            })
        })
    }

    /// Calls `mln_render_session_drain_frame_results`.
    pub fn drain_frame_results(&self) -> Result<RenderFrameBatchHandle> {
        let mut call = self.inner.call("mln_render_session_drain_frame_results")?;
        let mut out_batch = sys::mln_render_frame_batch(0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_drain_frame_results(session, &mut out_batch, out_diagnostic)
        })?;
        Ok(RenderFrameBatchHandle::adopt(out_batch, None)?)
    }

    /// Calls `mln_render_session_dump_debug_logs`.
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_dump_debug_logs")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_dump_debug_logs(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_render_session_get_capabilities`.
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

    /// Calls `mln_render_session_get_snapshot`.
    pub fn get_snapshot(&self) -> Result<RenderSessionSnapshot> {
        let mut call = self.inner.call("mln_render_session_get_snapshot")?;
        let mut out_snapshot: sys::mln_render_session_snapshot = unsafe { std::mem::zeroed() };
        out_snapshot.size = std::mem::size_of::<sys::mln_render_session_snapshot>() as _;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_get_snapshot(session, &mut out_snapshot, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_snapshot) }?)
    }

    /// Calls `mln_render_session_projection_create`.
    pub fn projection_create(&self) -> Result<MapProjectionHandle> {
        let mut call = self.inner.call("mln_render_session_projection_create")?;
        let mut out_projection = sys::mln_map_projection(0);
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_projection_create(session, &mut out_projection, out_diagnostic)
        })?;
        Ok(MapProjectionHandle::adopt(out_projection, None)?)
    }

    /// Calls `mln_render_session_query_feature_extensions`.
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

    /// Calls `mln_render_session_query_rendered_features`.
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

    /// Calls `mln_render_session_query_source_features`.
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

    /// Calls `mln_render_session_reduce_memory_use`.
    pub fn reduce_memory_use(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_render_session_reduce_memory_use")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_render_session_reduce_memory_use(session, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Calls `mln_render_session_request_frame`.
    pub fn request_frame(&self, demand: &FrameDemand) -> Result<()> {
        let mut call = self.inner.call("mln_render_session_request_frame")?;
        let demand = call.reference(&demand)?;
        call.status(|session, out_diagnostic| unsafe {
            sys::mln_render_session_request_frame(session, demand, out_diagnostic)
        })?;
        Ok(())
    }

    /// Calls `mln_render_session_resize`.
    pub fn resize(&self, extent: &RenderTargetExtent) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call("mln_render_session_resize")?;
        let extent = call.reference(&extent)?;
        call.command(|session, completion, out_diagnostic| unsafe {
            sys::mln_render_session_resize(session, extent, completion, out_diagnostic)
        })
    }

    /// Calls `mln_render_session_service_driver_work`.
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

    /// Calls `mln_texture_read_premultiplied_rgba8`.
    pub fn texture_read_premultiplied_rgba8(&self) -> Result<NativeFuture<TextureReadbackResult>> {
        let call = self.inner.call("mln_texture_read_premultiplied_rgba8")?;
        call.complete(
            |session, completion, out_diagnostic| unsafe {
                sys::mln_texture_read_premultiplied_rgba8(session, completion, out_diagnostic)
            },
            completion::value::<sys::mln_texture_readback_result, _>,
        )
    }

    /// Calls `mln_vulkan_borrowed_texture_set_target`.
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

    /// Calls `mln_vulkan_surface_set_target`.
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

    /// Calls `mln_webgpu_borrowed_texture_set_target`.
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

    /// Calls `mln_webgpu_surface_set_target`.
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
