// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_runtime` native handle.
    ///
    /// A runtime: the native scheduler thread and event store for its maps.
    ///
    /// See `mln_runtime` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RuntimeHandle(mln_runtime) dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_runtime_dispose(raw, out_diagnostic) });
}

impl RuntimeHandle {
    /// Creates a map on the runtime worker.
    ///
    /// See `mln_map_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn map_create(&self, options: &MapOptions) -> Result<NativeFuture<MapHandle>> {
        let mut call = self.inner.call("mln_map_create")?;
        let parent = self.inner.parent();
        let options = call.reference(&options)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_map_create(runtime, options, completion, out_diagnostic)
            },
            move |result| MapHandle::adopt(completion::copy_value::<sys::mln_map>(result)?, parent),
        )
    }

    /// Starts an ordered runtime barrier.
    ///
    /// See `mln_runtime_barrier` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_runtime_barrier")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_barrier(runtime, completion, out_diagnostic)
            },
            completion::unit,
        )
    }

    /// Clears the runtime-scoped outgoing HTTP header transform.
    ///
    /// See `mln_runtime_clear_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_http_header_transform(&self) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_runtime_clear_http_header_transform")?;
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
        let call = self.inner.call("mln_runtime_clear_resource_provider")?;
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
        let call = self.inner.call("mln_runtime_clear_resource_transform")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_clear_resource_transform(runtime, completion, out_diagnostic)
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

    /// Drains this runtime's queued events into a new owned batch.
    ///
    /// See `mln_runtime_drain_events` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn drain_events(&self) -> Result<Option<EventBatchHandle>> {
        let mut call = self.inner.call("mln_runtime_drain_events")?;
        let mut out_batch = sys::mln_event_batch(0);
        if !call.status_unless(
            sys::MLN_STATUS_NOT_READY,
            |runtime, out_diagnostic| unsafe {
                sys::mln_runtime_drain_events(runtime, &mut out_batch, out_diagnostic)
            },
        )? {
            return Ok(None);
        }
        Ok(Some(EventBatchHandle::adopt(out_batch, None)?))
    }

    /// Reports which runtime-scoped event types this runtime queues.
    ///
    /// See `mln_runtime_get_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn get_event_mask(&self) -> Result<RuntimeEventMask> {
        let mut call = self.inner.call("mln_runtime_get_event_mask")?;
        let mut out_mask: u64 = Default::default();
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_get_event_mask(runtime, &mut out_mask, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_mask) }?)
    }

    /// Starts creating an offline region.
    ///
    /// See `mln_runtime_offline_region_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_create(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let mut call = self.inner.call("mln_runtime_offline_region_create")?;
        let metadata_size = convert::count(metadata.len())?;
        let definition = call.reference(&definition)?;
        let metadata = metadata.as_ptr().cast();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_create(
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
    /// See `mln_runtime_offline_region_delete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_delete(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_runtime_offline_region_delete")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_delete(
                    runtime,
                    region_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Starts getting one offline region by ID.
    ///
    /// See `mln_runtime_offline_region_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_get(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<Option<OfflineRegionInfo>>> {
        let call = self.inner.call("mln_runtime_offline_region_get")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_get(runtime, region_id, completion, out_diagnostic)
            },
            completion::optional::<sys::mln_offline_region_info, _>,
        )
    }

    /// Starts getting the current download status for an offline region.
    ///
    /// See `mln_runtime_offline_region_get_status` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_get_status(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<OfflineRegionStatus>> {
        let call = self.inner.call("mln_runtime_offline_region_get_status")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_get_status(
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
    /// See `mln_runtime_offline_region_invalidate` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_invalidate(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_runtime_offline_region_invalidate")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_invalidate(
                    runtime,
                    region_id,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Sets an offline region's native download state.
    ///
    /// See `mln_runtime_offline_region_set_download_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_set_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<NativeFuture<()>> {
        let call = self
            .inner
            .call("mln_runtime_offline_region_set_download_state")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_set_download_state(
                    runtime,
                    region_id,
                    state.to_native(),
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Enables or disables runtime events for an offline region.
    ///
    /// See `mln_runtime_offline_region_set_observed` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_set_observed(
        &self,
        region_id: i64,
        observed: bool,
    ) -> Result<NativeFuture<()>> {
        let call = self.inner.call("mln_runtime_offline_region_set_observed")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_set_observed(
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

    /// Starts updating opaque binary metadata for an offline region.
    ///
    /// See `mln_runtime_offline_region_update_metadata` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_region_update_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let call = self
            .inner
            .call("mln_runtime_offline_region_update_metadata")?;
        let metadata_size = convert::count(metadata.len())?;
        let metadata = metadata.as_ptr().cast();
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_region_update_metadata(
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

    /// Starts listing the offline regions in the runtime database.
    ///
    /// See `mln_runtime_offline_regions_list` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_regions_list(&self) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let call = self.inner.call("mln_runtime_offline_regions_list")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_regions_list(runtime, completion, out_diagnostic)
            },
            completion::list::<sys::mln_offline_region_info, _>,
        )
    }

    /// Starts merging offline regions from another MapLibre offline database.
    ///
    /// See `mln_runtime_offline_regions_merge_database` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn offline_regions_merge_database(
        &self,
        side_database_path: &str,
    ) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let mut call = self
            .inner
            .call("mln_runtime_offline_regions_merge_database")?;
        let side_database_path = call.input(side_database_path)?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_offline_regions_merge_database(
                    runtime,
                    side_database_path,
                    completion,
                    out_diagnostic,
                )
            },
            completion::list::<sys::mln_offline_region_info, _>,
        )
    }

    /// Releases a runtime after synchronous child preflight.
    ///
    /// See `mln_runtime_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(&self) -> Result<NativeFuture<()>> {
        self.inner.release(|runtime| {
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
        let call = self.inner.call("mln_runtime_run_ambient_cache_operation")?;
        call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_run_ambient_cache_operation(
                    runtime,
                    operation.to_native(),
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )
    }

    /// Selects which runtime-scoped event types this runtime queues.
    ///
    /// See `mln_runtime_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_mask(&self, mask: RuntimeEventMask) -> Result<()> {
        let mut call = self.inner.call("mln_runtime_set_event_mask")?;
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_set_event_mask(runtime, mask.to_native(), out_diagnostic)
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
        let mut call = self.inner.call("mln_runtime_set_http_header_transform")?;
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
        let call = self
            .inner
            .call("mln_runtime_set_maximum_ambient_cache_size")?;
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

    /// Registers or replaces a runtime-scoped network resource provider.
    ///
    /// See `mln_runtime_set_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_provider(&self, provider: ResourceProvider) -> Result<NativeFuture<()>> {
        let mut call = self.inner.call("mln_runtime_set_resource_provider")?;
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
        let mut call = self.inner.call("mln_runtime_set_resource_transform")?;
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
}
