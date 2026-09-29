// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct RenderSessionHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_render_session>,
    id: u64,
    _parent: std::sync::Arc<crate::MapHandleState>,
}
impl RenderSessionHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_render_session> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("RenderSessionHandle"))
    }
}
impl Drop for RenderSessionHandleState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|raw| unsafe { maplibre_core::generated::render_session_dispose(raw) });
    }
}
/// Owns one `mln_render_session` native handle.
pub struct RenderSessionHandle {
    pub(crate) inner: std::sync::Arc<RenderSessionHandleState>,
}
impl std::fmt::Debug for RenderSessionHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RenderSessionHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl RenderSessionHandle {
    pub(crate) fn from_native(
        raw: sys::mln_render_session,
        parent: std::sync::Arc<crate::MapHandleState>,
    ) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle = unsafe {
            crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_render_session")
        }?;
        Ok(Self {
            inner: std::sync::Arc::new(RenderSessionHandleState {
                handle,
                id: raw.0,
                _parent: parent,
            }),
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

impl RenderSessionHandle {
    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_metal_borrowed_texture_set_target` using its header execution and ownership contract.
    pub unsafe fn metal_borrowed_texture_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::MetalBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_metal_borrowed_texture_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_metal_borrowed_texture_set_target(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_metal_surface_set_target` using its header execution and ownership contract.
    pub unsafe fn metal_surface_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::MetalSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_metal_surface_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_metal_surface_set_target(native, &binding_arg_1, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_opengl_borrowed_texture_set_target` using its header execution and ownership contract.
    pub unsafe fn opengl_borrowed_texture_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::OpenglBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_opengl_borrowed_texture_set_target", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_opengl_borrowed_texture_set_target(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_opengl_surface_set_target` using its header execution and ownership contract.
    pub unsafe fn opengl_surface_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::OpenglSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_opengl_surface_set_target", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_opengl_surface_set_target(native, &binding_arg_1, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_abandon` using its header execution and ownership contract.
    pub fn abandon(&self) -> Result<maplibre_core::generated::RenderAbandonResult> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_abandon", native.0)?;
        let mut binding_arg_1: sys::mln_render_abandon_result =
            maplibre_core::generated::RenderAbandonResult::default().to_native();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_abandon(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(maplibre_core::generated::RenderAbandonResult::from_native(
            binding_arg_1,
        ))
    }

    /// Calls `mln_render_session_acquire_frame` using its header execution and ownership contract.
    pub fn acquire_frame(&self) -> Result<crate::AcquiredFrameHandle> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_acquire_frame", native.0)?;
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let mut binding_arg_1 = sys::mln_acquired_frame(0);
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_acquire_frame(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(crate::AcquiredFrameHandle::from_native(
            binding_arg_1,
            binding_parent,
        )?)
    }

    /// Calls `mln_render_session_barrier` using its header execution and ownership contract.
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_barrier", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_barrier(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_clear_data` using its header execution and ownership contract.
    pub fn clear_data(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_clear_data", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_clear_data(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_destroy` using its header execution and ownership contract.
    pub fn destroy(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            maplibre_core::check(|diagnostic| unsafe {
                sys::mln_render_session_destroy(native, diagnostic)
            })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_render_session_detach` using its header execution and ownership contract.
    pub fn detach(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_detach", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_detach(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_dispose` using its header execution and ownership contract.
    pub fn dispose(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            maplibre_core::check(|diagnostic| unsafe {
                sys::mln_render_session_dispose(native, diagnostic)
            })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_render_session_drain_frame_results` using its header execution and ownership contract.
    pub fn drain_frame_results(&self) -> Result<crate::RenderFrameBatchHandle> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_drain_frame_results", native.0)?;
        let mut binding_arg_1 = sys::mln_render_frame_batch(0);
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_drain_frame_results(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(crate::RenderFrameBatchHandle::from_native(binding_arg_1)?)
    }

    /// Calls `mln_render_session_dump_debug_logs` using its header execution and ownership contract.
    pub fn dump_debug_logs(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_dump_debug_logs", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_dump_debug_logs(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_get_capabilities` using its header execution and ownership contract.
    pub fn get_capabilities(&self) -> Result<maplibre_core::generated::RenderSessionCapabilities> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_get_capabilities", native.0)?;
        let mut binding_arg_1: sys::mln_render_session_capabilities =
            maplibre_core::generated::RenderSessionCapabilities::default().to_native();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_get_capabilities(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(maplibre_core::generated::RenderSessionCapabilities::from_native(binding_arg_1))
    }

    /// Calls `mln_render_session_get_snapshot` using its header execution and ownership contract.
    pub fn get_snapshot(&self) -> Result<maplibre_core::generated::RenderSessionSnapshot> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_get_snapshot", native.0)?;
        let mut binding_arg_1: sys::mln_render_session_snapshot =
            maplibre_core::generated::RenderSessionSnapshot::default().to_native();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_get_snapshot(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(maplibre_core::generated::RenderSessionSnapshot::from_native(binding_arg_1))
    }

    /// Calls `mln_render_session_projection_create` using its header execution and ownership contract.
    pub fn projection_create(&self) -> Result<crate::MapProjectionHandle> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_projection_create", native.0)?;
        let mut binding_arg_1 = sys::mln_map_projection(0);
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_projection_create(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(crate::MapProjectionHandle::from_native(binding_arg_1)?)
    }

    /// Calls `mln_render_session_query_feature_extensions` using its header execution and ownership contract.
    pub fn query_feature_extensions(
        &self,
        binding_arg_1: &str,
        binding_arg_2: &[u8],
        binding_arg_3: &str,
        binding_arg_4: &str,
        binding_arg_5: Option<&[u8]>,
    ) -> Result<NativeFuture<Vec<u8>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_query_feature_extensions", native.0)?;
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_2).as_ptr().cast(),
            size: (binding_arg_2).len(),
        };
        let binding_arg_3 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_3).as_bytes().as_ptr().cast(),
            size: (binding_arg_3).as_bytes().len(),
        };
        let binding_arg_4 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_4).as_bytes().as_ptr().cast(),
            size: (binding_arg_4).as_bytes().len(),
        };
        let binding_arg_5 = binding_arg_5
            .map(|value| -> Result<_> {
                Ok(maplibre_native_ffi_sys::mln_buffer_view {
                    data: (value).as_ptr().cast(),
                    size: (value).len(),
                })
            })
            .transpose()?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_query_feature_extensions(
                    native,
                    binding_arg_1,
                    binding_arg_2,
                    binding_arg_3,
                    binding_arg_4,
                    binding_arg_5
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                    diagnostic,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_buffer_view>(result)?;
                Ok(unsafe { maplibre_core::string::copy_string_view_bytes(value) }?)
            },
        )
    }

    /// Calls `mln_render_session_query_rendered_features` using its header execution and ownership contract.
    pub fn query_rendered_features(
        &self,
        binding_arg_1: &maplibre_core::generated::RenderedQueryGeometry,
        binding_arg_2: Option<&maplibre_core::generated::RenderedFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::QueriedFeature>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_query_rendered_features", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2 = binding_arg_2
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_query_rendered_features(
                    native,
                    &binding_arg_1,
                    binding_arg_2
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                    diagnostic,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_queried_feature>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(
                            unsafe {
                                maplibre_core::generated::QueriedFeature::from_native(value)
                            }?,
                        )
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_render_session_query_source_features` using its header execution and ownership contract.
    pub fn query_source_features(
        &self,
        binding_arg_1: &str,
        binding_arg_2: Option<&maplibre_core::generated::SourceFeatureQueryOptions>,
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::QueriedFeature>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_query_source_features", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = maplibre_native_ffi_sys::mln_buffer_view {
            data: (binding_arg_1).as_bytes().as_ptr().cast(),
            size: (binding_arg_1).as_bytes().len(),
        };
        let binding_arg_2 = binding_arg_2
            .map(|value| value.to_native(&mut arena))
            .transpose()?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_query_source_features(
                    native,
                    binding_arg_1,
                    binding_arg_2
                        .as_ref()
                        .map_or(std::ptr::null(), |value| value),
                    completion,
                    diagnostic,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_queried_feature>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(
                            unsafe {
                                maplibre_core::generated::QueriedFeature::from_native(value)
                            }?,
                        )
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_render_session_reduce_memory_use` using its header execution and ownership contract.
    pub fn reduce_memory_use(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_reduce_memory_use", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_render_session_reduce_memory_use(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_render_session_request_frame` using its header execution and ownership contract.
    pub fn request_frame(
        &self,
        binding_arg_1: &maplibre_core::generated::FrameDemand,
    ) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_request_frame", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_request_frame(native, &binding_arg_1, diagnostic)
        })?;
        Ok(())
    }

    /// Calls `mln_render_session_resize` using its header execution and ownership contract.
    pub fn resize(
        &self,
        binding_arg_1: &maplibre_core::generated::RenderTargetExtent,
    ) -> Result<NativeFuture<crate::CommandCompletion>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_resize", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit_command(|completion, diagnostic| unsafe {
            sys::mln_render_session_resize(native, &binding_arg_1, completion, diagnostic)
        })
    }

    /// Calls `mln_render_session_service_driver_work` using its header execution and ownership contract.
    pub fn service_driver_work(&self, binding_arg_1: usize) -> Result<usize> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_render_session_service_driver_work", native.0)?;
        let mut binding_arg_2: usize = Default::default();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_render_session_service_driver_work(
                native,
                binding_arg_1,
                &mut binding_arg_2,
                diagnostic,
            )
        })?;
        Ok(binding_arg_2)
    }

    /// Calls `mln_texture_read_premultiplied_rgba8` using its header execution and ownership contract.
    pub fn texture_read_premultiplied_rgba8(
        &self,
    ) -> Result<NativeFuture<maplibre_core::generated::TextureReadbackResult>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_texture_read_premultiplied_rgba8", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_texture_read_premultiplied_rgba8(native, completion, diagnostic)
            },
            |result| {
                let value =
                    crate::completion::copy_value::<sys::mln_texture_readback_result>(result)?;
                Ok(unsafe { maplibre_core::generated::TextureReadbackResult::from_native(value) }?)
            },
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_vulkan_borrowed_texture_set_target` using its header execution and ownership contract.
    pub unsafe fn vulkan_borrowed_texture_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::VulkanBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_vulkan_borrowed_texture_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_vulkan_borrowed_texture_set_target(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_vulkan_surface_set_target` using its header execution and ownership contract.
    pub unsafe fn vulkan_surface_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::VulkanSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_vulkan_surface_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_vulkan_surface_set_target(native, &binding_arg_1, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_webgpu_borrowed_texture_set_target` using its header execution and ownership contract.
    pub unsafe fn webgpu_borrowed_texture_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::WebgpuBorrowedTextureDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_webgpu_borrowed_texture_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_webgpu_borrowed_texture_set_target(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// # Safety
    /// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
    /// Calls `mln_webgpu_surface_set_target` using its header execution and ownership contract.
    pub unsafe fn webgpu_surface_set_target(
        &self,
        binding_arg_1: &maplibre_core::generated::WebgpuSurfaceDescriptor,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_webgpu_surface_set_target", native.0)?;
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_webgpu_surface_set_target(native, &binding_arg_1, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }
}
