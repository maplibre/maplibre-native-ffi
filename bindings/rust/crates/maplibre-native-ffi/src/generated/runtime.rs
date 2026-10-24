// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_runtime` native handle.
    ///
    /// A runtime: the native scheduler thread and event store for its maps.
    ///
    /// See `mln_runtime` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RuntimeHandle(mln_runtime) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_runtime_dispose(raw, out_diagnostic) }), counted false;
}

impl RuntimeHandle {
    /// Stops event delivery and retires the previous handler.
    ///
    /// See `mln_runtime_clear_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_event_handler(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_clear_event_handler(runtime, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Clears the runtime-scoped outgoing HTTP header transform.
    ///
    /// See `mln_runtime_clear_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_http_header_transform(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_clear_http_header_transform(runtime, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Clears the runtime-scoped network resource provider.
    ///
    /// See `mln_runtime_clear_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_provider(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_clear_resource_provider(runtime, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Clears the runtime-scoped URL transform for network resources.
    ///
    /// See `mln_runtime_clear_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_transform(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_clear_resource_transform(runtime, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Starts creation of a map on the runtime.
    ///
    /// See `mln_runtime_create_map` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn create_map(&self, options: &MapOptions) -> Result<NativeFuture<MapHandle>> {
        let mut call = self.inner.call();
        let parent = self.inner.parent();
        let options = call.reference(&options)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_create_map(runtime, options, completion, out_diagnostic)
            },
            move |result| MapHandle::adopt(completion::copy_value::<sys::mln_map>(result)?, parent),
        )
    }

    /// Starts creating an offline region.
    ///
    /// See `mln_runtime_create_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let mut call = self.inner.call();
        let metadata_size = convert::count(metadata.len())?;
        let definition = call.reference(&definition)?;
        let metadata = metadata.as_ptr().cast();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_create_offline_region(
                    runtime,
                    definition,
                    metadata,
                    metadata_size,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_offline_region_info, _>,
        )
    }

    /// Deletes an offline region.
    ///
    /// See `mln_runtime_delete_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn delete_offline_region(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_delete_offline_region(
                    runtime,
                    region_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Consumes a runtime handle without observing its asynchronous retirement.
    ///
    /// See `mln_runtime_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn dispose(&self) -> Result<()> {
        self.inner.close(|runtime| {
            let mut call = Call::new(runtime, None);
            call.status(|runtime, out_diagnostic| unsafe {
                sys::mln_runtime_dispose(runtime, out_diagnostic)
            })
        })
    }

    /// Reports which runtime-scoped event types this runtime queues.
    ///
    /// See `mln_runtime_get_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn get_event_mask(&self) -> Result<RuntimeEventMask> {
        let mut call = self.inner.call();
        let mut out_mask: u64 = Default::default();
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_get_event_mask(runtime, &mut out_mask, out_diagnostic)
        })?;
        Ok(RuntimeEventMask::from_bits_retain(out_mask))
    }

    /// Starts getting one offline region by ID.
    ///
    /// See `mln_runtime_get_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_offline_region(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<Option<OfflineRegionInfo>>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_get_offline_region(runtime, region_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_offline_region_info, _>,
        )
    }

    /// Starts getting the current download status for an offline region.
    ///
    /// See `mln_runtime_get_offline_region_status` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn get_offline_region_status(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<OfflineRegionStatus>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_get_offline_region_status(
                    runtime,
                    region_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_offline_region_status, _>,
        )
    }

    /// Invalidates cached resources for an offline region.
    ///
    /// See `mln_runtime_invalidate_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn invalidate_offline_region(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_invalidate_offline_region(
                    runtime,
                    region_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts listing the offline regions in the runtime database.
    ///
    /// See `mln_runtime_list_offline_regions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn list_offline_regions(&self) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_list_offline_regions(runtime, completion, out_diagnostic)
            },
            completion::list::<sys::mln_offline_region_info, _>,
        )
    }

    /// Starts merging offline regions from another MapLibre offline database.
    ///
    /// See `mln_runtime_merge_offline_regions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn merge_offline_regions(
        &self,
        side_database_path: &str,
    ) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let mut call = self.inner.call();
        let side_database_path = call.input(&side_database_path)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_merge_offline_regions(
                    runtime,
                    side_database_path,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_offline_region_info, _>,
        )
    }

    /// Starts release of a runtime after checking its children.
    ///
    /// See `mln_runtime_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(&self) -> Result<NativeFuture<()>> {
        self.inner.close(|runtime| {
            let call = Call::new(runtime, None);
            call.complete(
                |runtime, completion, out_diagnostic| unsafe {
                    sys::mln_runtime_release(runtime, completion, out_diagnostic)
                },
                completion::unit,
            )
        })
    }

    /// Starts a MapLibre ambient cache maintenance operation for this runtime.
    ///
    /// See `mln_runtime_run_ambient_cache_operation` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn run_ambient_cache_operation(
        &self,
        operation: AmbientCacheOperation,
    ) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_run_ambient_cache_operation(
                    runtime,
                    operation.as_raw(),
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Selects how runtime and map events reach the host.
    ///
    /// See `mln_runtime_set_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_handler(&self, handler: RuntimeEventHandler) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let handler = call.reference(&handler)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_event_handler(runtime, handler, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Selects which runtime-scoped event types this runtime queues.
    ///
    /// See `mln_runtime_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_mask(&self, mask: RuntimeEventMask) -> Result<()> {
        let mut call = self.inner.call();
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_set_event_mask(runtime, mask.bits(), out_diagnostic)
        })?;
        Ok(())
    }

    /// Registers or replaces the runtime-scoped outgoing HTTP header transform.
    ///
    /// See `mln_runtime_set_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_http_header_transform(
        &self,
        transform: HttpHeaderTransform,
    ) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let transform = call.reference(&transform)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_http_header_transform(
                    runtime,
                    transform,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts a change to this runtime's maximum ambient cache size.
    ///
    /// See `mln_runtime_set_maximum_ambient_cache_size` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_maximum_ambient_cache_size(&self, size: u64) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_maximum_ambient_cache_size(
                    runtime,
                    size,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Sets an offline region's native download state.
    ///
    /// See `mln_runtime_set_offline_region_download_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_offline_region_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_offline_region_download_state(
                    runtime,
                    region_id,
                    state.as_raw(),
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Enables or disables runtime events for an offline region.
    ///
    /// See `mln_runtime_set_offline_region_observed` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn set_offline_region_observed(
        &self,
        region_id: i64,
        observed: bool,
    ) -> Result<NativeFuture<()>> {
        let call = self.inner.call();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_offline_region_observed(
                    runtime,
                    region_id,
                    observed,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Registers or replaces a runtime-scoped network resource provider.
    ///
    /// See `mln_runtime_set_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_provider(&self, provider: ResourceProvider) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let provider = call.reference(&provider)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_resource_provider(
                    runtime,
                    provider,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Registers or updates a runtime-scoped URL transform for network
    /// resources.
    ///
    /// See `mln_runtime_set_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_transform(&self, transform: ResourceTransform) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call();
        let transform = call.reference(&transform)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_set_resource_transform(
                    runtime,
                    transform,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts updating opaque binary metadata for an offline region.
    ///
    /// See `mln_runtime_update_offline_region_metadata` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn update_offline_region_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let call = self.inner.call();
        let metadata_size = convert::count(metadata.len())?;
        let metadata = metadata.as_ptr().cast();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_update_offline_region_metadata(
                    runtime,
                    region_id,
                    metadata,
                    metadata_size,
                    completion,
                    out_diagnostic,
                )
            },
            completion::value::<sys::mln_offline_region_info, _>,
        )
    }
}
