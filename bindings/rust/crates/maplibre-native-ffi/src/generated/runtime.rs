// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

#[derive(Debug)]
pub(crate) struct RuntimeHandleState {
    pub(crate) handle: crate::handle::ConcurrentNativeHandle<sys::mln_runtime>,
    id: u64,
}
impl RuntimeHandleState {
    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| crate::handle::closed_handle_error("RuntimeHandle"))
    }
}
impl Drop for RuntimeHandleState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|raw| unsafe { maplibre_core::generated::runtime_dispose(raw) });
    }
}
/// Owns one `mln_runtime` native handle.
pub struct RuntimeHandle {
    pub(crate) inner: std::sync::Arc<RuntimeHandleState>,
}
impl std::fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.is_closed())
            .finish()
    }
}
impl RuntimeHandle {
    pub(crate) fn from_native(raw: sys::mln_runtime) -> Result<Self> {
        // SAFETY: raw came from an accepted ownership transfer of this handle type.
        let handle =
            unsafe { crate::handle::ConcurrentNativeHandle::from_handle(raw, "mln_runtime") }?;
        Ok(Self {
            inner: std::sync::Arc::new(RuntimeHandleState { handle, id: raw.0 }),
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

impl RuntimeHandle {
    /// Calls `mln_map_create` using its header execution and ownership contract.
    pub fn map_create(
        &self,
        binding_arg_1: &maplibre_core::generated::MapOptions,
    ) -> Result<NativeFuture<crate::MapHandle>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_map_create", native.0)?;
        let binding_parent = std::sync::Arc::clone(&self.inner);
        let binding_arg_1 = binding_arg_1.to_native();
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_map_create(native, &binding_arg_1, completion, diagnostic)
            },
            move |result| {
                let value = crate::completion::copy_value::<sys::mln_map>(result)?;
                crate::MapHandle::from_native(value, binding_parent)
            },
        )
    }

    /// Calls `mln_runtime_barrier` using its header execution and ownership contract.
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_barrier", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_barrier(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_clear_http_header_transform` using its header execution and ownership contract.
    pub fn clear_http_header_transform(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_clear_http_header_transform", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_clear_http_header_transform(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_clear_resource_provider` using its header execution and ownership contract.
    pub fn clear_resource_provider(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_clear_resource_provider", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_clear_resource_provider(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_clear_resource_transform` using its header execution and ownership contract.
    pub fn clear_resource_transform(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_clear_resource_transform", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_clear_resource_transform(native, completion, diagnostic)
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_dispose` using its header execution and ownership contract.
    pub fn dispose(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            maplibre_core::check(|diagnostic| unsafe {
                sys::mln_runtime_dispose(native, diagnostic)
            })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_runtime_drain_events` using its header execution and ownership contract.
    pub fn drain_events(&self) -> Result<crate::EventBatchHandle> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_drain_events", native.0)?;
        let mut binding_arg_1 = sys::mln_event_batch(0);
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_runtime_drain_events(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(crate::EventBatchHandle::from_native(binding_arg_1)?)
    }

    /// Calls `mln_runtime_get_event_mask` using its header execution and ownership contract.
    pub fn get_event_mask(&self) -> Result<maplibre_core::generated::RuntimeEventMask> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_get_event_mask", native.0)?;
        let mut binding_arg_1: u64 = Default::default();
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_runtime_get_event_mask(native, &mut binding_arg_1, diagnostic)
        })?;
        Ok(maplibre_core::generated::RuntimeEventMask::from_native(
            binding_arg_1,
        ))
    }

    /// Calls `mln_runtime_offline_region_create` using its header execution and ownership contract.
    pub fn offline_region_create(
        &self,
        binding_arg_1: &maplibre_core::generated::OfflineRegionDefinition,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<maplibre_core::generated::OfflineRegionInfo>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_create", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let binding_arg_2_native = (binding_arg_2).as_ptr().cast();
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_create(
                    native,
                    &binding_arg_1,
                    binding_arg_2_native,
                    binding_arg_3,
                    completion,
                    diagnostic,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_offline_region_info>(result)?;
                Ok(unsafe { maplibre_core::generated::OfflineRegionInfo::from_native(value) }?)
            },
        )
    }

    /// Calls `mln_runtime_offline_region_delete` using its header execution and ownership contract.
    pub fn offline_region_delete(&self, binding_arg_1: i64) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_delete", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_delete(
                    native,
                    binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_offline_region_get` using its header execution and ownership contract.
    pub fn offline_region_get(
        &self,
        binding_arg_1: i64,
    ) -> Result<NativeFuture<Option<maplibre_core::generated::OfflineRegionInfo>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_get", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_get(native, binding_arg_1, completion, diagnostic)
            },
            |result| {
                crate::completion::optional_value::<sys::mln_offline_region_info>(result)?
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::OfflineRegionInfo::from_native(value)
                        }?)
                    })
                    .transpose()
            },
        )
    }

    /// Calls `mln_runtime_offline_region_get_status` using its header execution and ownership contract.
    pub fn offline_region_get_status(
        &self,
        binding_arg_1: i64,
    ) -> Result<NativeFuture<maplibre_core::generated::OfflineRegionStatus>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_get_status", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_get_status(
                    native,
                    binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            |result| {
                let value =
                    crate::completion::copy_value::<sys::mln_offline_region_status>(result)?;
                Ok(maplibre_core::generated::OfflineRegionStatus::from_native(
                    value,
                ))
            },
        )
    }

    /// Calls `mln_runtime_offline_region_invalidate` using its header execution and ownership contract.
    pub fn offline_region_invalidate(&self, binding_arg_1: i64) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_invalidate", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_invalidate(
                    native,
                    binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_offline_region_set_download_state` using its header execution and ownership contract.
    pub fn offline_region_set_download_state(
        &self,
        binding_arg_1: i64,
        binding_arg_2: maplibre_core::generated::OfflineRegionDownloadState,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_set_download_state", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_set_download_state(
                    native,
                    binding_arg_1,
                    binding_arg_2.to_native(),
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_offline_region_set_observed` using its header execution and ownership contract.
    pub fn offline_region_set_observed(
        &self,
        binding_arg_1: i64,
        binding_arg_2: bool,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_set_observed", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_set_observed(
                    native,
                    binding_arg_1,
                    binding_arg_2,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_offline_region_update_metadata` using its header execution and ownership contract.
    pub fn offline_region_update_metadata(
        &self,
        binding_arg_1: i64,
        binding_arg_2: &[u8],
    ) -> Result<NativeFuture<maplibre_core::generated::OfflineRegionInfo>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_region_update_metadata", native.0)?;
        let binding_arg_2_native = (binding_arg_2).as_ptr().cast();
        let binding_arg_3 = binding_arg_2
            .len()
            .try_into()
            .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_region_update_metadata(
                    native,
                    binding_arg_1,
                    binding_arg_2_native,
                    binding_arg_3,
                    completion,
                    diagnostic,
                )
            },
            |result| {
                let value = crate::completion::copy_value::<sys::mln_offline_region_info>(result)?;
                Ok(unsafe { maplibre_core::generated::OfflineRegionInfo::from_native(value) }?)
            },
        )
    }

    /// Calls `mln_runtime_offline_regions_list` using its header execution and ownership contract.
    pub fn offline_regions_list(
        &self,
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::OfflineRegionInfo>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_regions_list", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_regions_list(native, completion, diagnostic)
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_offline_region_info>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::OfflineRegionInfo::from_native(value)
                        }?)
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_runtime_offline_regions_merge_database` using its header execution and ownership contract.
    pub fn offline_regions_merge_database(
        &self,
        binding_arg_1: &str,
    ) -> Result<NativeFuture<Vec<maplibre_core::generated::OfflineRegionInfo>>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_offline_regions_merge_database", native.0)?;
        let binding_arg_1 = maplibre_core::string::c_string(binding_arg_1)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_offline_regions_merge_database(
                    native,
                    binding_arg_1.as_ptr(),
                    completion,
                    diagnostic,
                )
            },
            |result| {
                crate::completion::copy_slice::<sys::mln_offline_region_info>(result)?
                    .into_iter()
                    .map(|value| -> Result<_> {
                        Ok(unsafe {
                            maplibre_core::generated::OfflineRegionInfo::from_native(value)
                        }?)
                    })
                    .collect::<Result<Vec<_>>>()
            },
        )
    }

    /// Calls `mln_runtime_release` using its header execution and ownership contract.
    pub fn release(&self) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.inner.handle.close_with(|native| {
            crate::completion::submit(
                |completion, diagnostic| unsafe {
                    sys::mln_runtime_release(native, completion, diagnostic)
                },
                crate::completion::unit,
            )
        })?;
        Ok(result.unwrap_or_else(|| crate::completion::ready(())))
    }

    /// Calls `mln_runtime_run_ambient_cache_operation` using its header execution and ownership contract.
    pub fn run_ambient_cache_operation(
        &self,
        binding_arg_1: maplibre_core::generated::AmbientCacheOperation,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_run_ambient_cache_operation", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_run_ambient_cache_operation(
                    native,
                    binding_arg_1.to_native(),
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_set_event_mask` using its header execution and ownership contract.
    pub fn set_event_mask(
        &self,
        binding_arg_1: maplibre_core::generated::RuntimeEventMask,
    ) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_set_event_mask", native.0)?;
        maplibre_core::check(|diagnostic| unsafe {
            sys::mln_runtime_set_event_mask(native, binding_arg_1.to_native(), diagnostic)
        })?;
        Ok(())
    }

    /// Calls `mln_runtime_set_http_header_transform` using its header execution and ownership contract.
    pub fn set_http_header_transform(
        &self,
        binding_arg_1: maplibre_core::generated::HttpHeaderTransform,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_set_http_header_transform", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let submitted = crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_set_http_header_transform(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        );
        if submitted.is_ok() {
            arena.accept_registrations();
        }
        submitted
    }

    /// Calls `mln_runtime_set_maximum_ambient_cache_size` using its header execution and ownership contract.
    pub fn set_maximum_ambient_cache_size(&self, binding_arg_1: u64) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_set_maximum_ambient_cache_size", native.0)?;
        crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_set_maximum_ambient_cache_size(
                    native,
                    binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        )
    }

    /// Calls `mln_runtime_set_resource_provider` using its header execution and ownership contract.
    pub fn set_resource_provider(
        &self,
        binding_arg_1: maplibre_core::generated::ResourceProvider,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_set_resource_provider", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let submitted = crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_set_resource_provider(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        );
        if submitted.is_ok() {
            arena.accept_registrations();
        }
        submitted
    }

    /// Calls `mln_runtime_set_resource_transform` using its header execution and ownership contract.
    pub fn set_resource_transform(
        &self,
        binding_arg_1: maplibre_core::generated::ResourceTransform,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.inner.native()?;
        maplibre_core::callback::check("mln_runtime_set_resource_transform", native.0)?;
        let mut arena = maplibre_core::input::InputArena::default();
        let binding_arg_1 = binding_arg_1.to_native(&mut arena)?;
        let submitted = crate::completion::submit(
            |completion, diagnostic| unsafe {
                sys::mln_runtime_set_resource_transform(
                    native,
                    &binding_arg_1,
                    completion,
                    diagnostic,
                )
            },
            crate::completion::unit,
        );
        if submitted.is_ok() {
            arena.accept_registrations();
        }
        submitted
    }
}
