// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A runtime owns maps and the network and cache settings that they share.
    ///
    /// See `mln_runtime` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RuntimeHandle(mln_runtime) dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_runtime_dispose(raw, out_diagnostic) });
    /// Returns the native handle value, which runtime events report as their source.
    fn id;
}

impl RuntimeHandle {
    /// Stops sending the events of the runtime and its maps to a callback.
    ///
    /// See `mln_runtime_clear_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_event_handler(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_event_handler(runtime, completion, out_diagnostic)
        })
    }

    /// Stops adding headers to the runtime's HTTP requests.
    ///
    /// See `mln_runtime_clear_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_http_header_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_http_header_transform(runtime, completion, out_diagnostic)
        })
    }

    /// Removes the runtime's resource provider, so the library loads every
    /// request itself.
    ///
    /// See `mln_runtime_clear_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_provider(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_resource_provider(runtime, completion, out_diagnostic)
        })
    }

    /// Stops rewriting the URLs of the runtime's network requests.
    ///
    /// See `mln_runtime_clear_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        let call = self.inner.call();
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_resource_transform(runtime, completion, out_diagnostic)
        })
    }

    /// Creates a runtime.
    ///
    /// A runtime holds maps and the resource loading, cache, and offline
    /// regions that they share. In a browser, create runtimes on a Web Worker,
    /// not on the main thread.
    ///
    /// See `mln_runtime_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn create(options: &RuntimeOptions<'_>) -> Result<RuntimeHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_runtime = sys::mln_runtime(0);
        let options = call.reference(&options)?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_runtime_create(options, &mut out_runtime, out_diagnostic)
        })?;
        Ok(RuntimeHandle::adopt(out_runtime)?)
    }

    /// Creates a map on `runtime`.
    ///
    /// See `mln_runtime_create_map` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    pub fn create_map(&self, options: &MapOptions) -> Result<(MapHandle, NativeFuture<()>)> {
        let mut call = self.inner.call();
        let mut out_map = sys::mln_map(0);
        let options = call.reference(&options)?;
        let future = call.complete(
            |runtime, completion, out_diagnostic| unsafe {
                sys::mln_runtime_create_map(
                    runtime,
                    options,
                    &mut out_map,
                    completion,
                    out_diagnostic,
                )
            },
            completion::unit,
        )?;
        Ok((MapHandle::adopt(out_map)?, future))
    }

    /// Creates an offline region in the runtime's cache file.
    ///
    /// `metadata` holds any bytes to store with the region, such as its name.
    /// The region doesn't download until
    /// `RuntimeHandle::set_offline_region_download_state()` makes its download
    /// active.
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

    /// Deletes the offline region `region_id` from the runtime's cache file.
    ///
    /// See `mln_runtime_delete_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

    /// Releases a runtime in any state, without waiting.
    ///
    /// The runtime finishes its teardown after its maps are released.
    ///
    /// See `mln_runtime_dispose` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn dispose(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|runtime| {
            let mut call = Call::new(runtime);
            call.status(|runtime, out_diagnostic| unsafe {
                sys::mln_runtime_dispose(runtime, out_diagnostic)
            })
        })
    }

    /// Reads the runtime's event mask.
    ///
    /// See `mln_runtime_get_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn get_event_mask(&self) -> Result<RuntimeEventFlag> {
        let mut call = self.inner.call();
        let mut out_mask: sys::mln_runtime_event_flags = Default::default();
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_get_event_mask(runtime, &mut out_mask, out_diagnostic)
        })?;
        Ok(RuntimeEventFlag::from_bits_retain(out_mask))
    }

    /// Reads the offline region `region_id`.
    ///
    /// The result is null when the cache file has no region with that ID.
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

    /// Reads the download progress of the offline region `region_id`.
    ///
    /// See `mln_runtime_get_offline_region_status` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

    /// Makes maps check each resource of the offline region `region_id` with
    /// the server before they use it.
    ///
    /// The region keeps its resources.
    ///
    /// See `mln_runtime_invalidate_offline_region` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

    /// Lists the offline regions in the runtime's cache file.
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

    /// Copies the offline regions of another cache file into the runtime's
    /// cache file.
    ///
    /// A region whose definition and metadata match one that the runtime's file
    /// already has isn't copied again. The result lists the runtime's copy of
    /// each region in the other file.
    ///
    /// See `mln_runtime_merge_offline_regions` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidArgument` when `side_database_path` isn't a readable
    ///   database file.
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

    /// Releases a runtime.
    ///
    /// Release the runtime's maps first.
    ///
    /// See `mln_runtime_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(self) -> std::result::Result<NativeFuture<()>, (Self, Error)> {
        self.consume(|runtime| {
            let call = Call::new(runtime);
            call.complete(
                |runtime, completion, out_diagnostic| unsafe {
                    sys::mln_runtime_release(runtime, completion, out_diagnostic)
                },
                completion::unit,
            )
        })
    }

    /// Runs an action on the runtime's ambient cache.
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

    /// Sends the events of the runtime and its maps to a callback.
    ///
    /// The callback replaces the one set before.
    ///
    /// See `mln_runtime_set_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_handler(
        &self,
        handler: RuntimeEventHandler,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let handler = call.reference(&handler)?;
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_event_handler(runtime, handler, completion, out_diagnostic)
        })
    }

    /// Selects the runtime's own events that reach the event callback.
    ///
    /// Each map selects its events with `MapHandle::set_event_mask()` instead.
    /// A runtime starts with every event selected.
    ///
    /// See `mln_runtime_set_event_mask` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_mask(&self, mask: RuntimeEventFlag) -> Result<()> {
        let mut call = self.inner.call();
        call.status(|runtime, out_diagnostic| unsafe {
            sys::mln_runtime_set_event_mask(runtime, mask.bits(), out_diagnostic)
        })?;
        Ok(())
    }

    /// Adds headers to the runtime's HTTP requests.
    ///
    /// The transform replaces the one set before. In a browser or on
    /// OpenHarmony, serve the requests with
    /// `RuntimeHandle::set_resource_provider()` instead.
    ///
    /// Runs on a library thread.
    ///
    /// See `mln_runtime_set_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the library runs in a browser or on
    ///   OpenHarmony.
    pub fn set_http_header_transform(
        &self,
        transform: HttpHeaderTransform,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let transform = call.reference(&transform)?;
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_http_header_transform(
                runtime,
                transform,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Limits the size of the runtime's ambient cache.
    ///
    /// `size` is in bytes. Offline regions count toward it, but the library
    /// evicts only ambient cache resources to stay within it.
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

    /// Starts or pauses the download of the offline region `region_id`.
    ///
    /// A download stops when the runtime closes.
    ///
    /// See `mln_runtime_set_offline_region_download_state` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

    /// Sets whether the runtime reports the download progress of the offline
    /// region `region_id` as events.
    ///
    /// See `mln_runtime_set_offline_region_observed` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

    /// Serves the runtime's network requests through a callback.
    ///
    /// The provider receives the network requests that its routes match, and
    /// the library loads the rest as usual. A provider can serve URL schemes
    /// that the library can't load itself. The provider replaces the one set
    /// before.
    ///
    /// See `mln_runtime_set_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_provider(
        &self,
        provider: ResourceProvider,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let provider = call.reference(&provider)?;
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_resource_provider(runtime, provider, completion, out_diagnostic)
        })
    }

    /// Rewrites the URLs of the runtime's network requests through a callback.
    ///
    /// The transform replaces the one set before.
    ///
    /// See `mln_runtime_set_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_transform(
        &self,
        transform: ResourceTransform,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let mut call = self.inner.call();
        let transform = call.reference(&transform)?;
        call.command(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_resource_transform(runtime, transform, completion, out_diagnostic)
        })
    }

    /// Replaces the metadata of the offline region `region_id`.
    ///
    /// See `mln_runtime_update_offline_region_metadata` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::NotFound` when the cache file has no region `region_id`.
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

/// Submits [`RuntimeHandle`] commands without a completion. Each call returns
/// the same acceptance status as its completing form, and native logs a
/// command that later fails as a warning through the log handler.
pub struct RuntimeHandleDiscarding<'a>(&'a RuntimeHandle);
impl RuntimeHandle {
    pub fn discarding(&self) -> RuntimeHandleDiscarding<'_> {
        RuntimeHandleDiscarding(self)
    }
}
impl RuntimeHandleDiscarding<'_> {
    /// Stops sending the events of the runtime and its maps to a callback.
    ///
    /// See `mln_runtime_clear_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_event_handler(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_event_handler(runtime, completion, out_diagnostic)
        })
    }

    /// Stops adding headers to the runtime's HTTP requests.
    ///
    /// See `mln_runtime_clear_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_http_header_transform(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_http_header_transform(runtime, completion, out_diagnostic)
        })
    }

    /// Removes the runtime's resource provider, so the library loads every
    /// request itself.
    ///
    /// See `mln_runtime_clear_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_provider(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_resource_provider(runtime, completion, out_diagnostic)
        })
    }

    /// Stops rewriting the URLs of the runtime's network requests.
    ///
    /// See `mln_runtime_clear_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn clear_resource_transform(&self) -> Result<()> {
        let call = self.0.inner.call();
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_clear_resource_transform(runtime, completion, out_diagnostic)
        })
    }

    /// Sends the events of the runtime and its maps to a callback.
    ///
    /// The callback replaces the one set before.
    ///
    /// See `mln_runtime_set_event_handler` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_event_handler(&self, handler: RuntimeEventHandler) -> Result<()> {
        let mut call = self.0.inner.call();
        let handler = call.reference(&handler)?;
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_event_handler(runtime, handler, completion, out_diagnostic)
        })
    }

    /// Adds headers to the runtime's HTTP requests.
    ///
    /// The transform replaces the one set before. In a browser or on
    /// OpenHarmony, serve the requests with
    /// `RuntimeHandle::set_resource_provider()` instead.
    ///
    /// Runs on a library thread.
    ///
    /// See `mln_runtime_set_http_header_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when the library runs in a browser or on
    ///   OpenHarmony.
    pub fn set_http_header_transform(&self, transform: HttpHeaderTransform) -> Result<()> {
        let mut call = self.0.inner.call();
        let transform = call.reference(&transform)?;
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_http_header_transform(
                runtime,
                transform,
                completion,
                out_diagnostic,
            )
        })
    }

    /// Serves the runtime's network requests through a callback.
    ///
    /// The provider receives the network requests that its routes match, and
    /// the library loads the rest as usual. A provider can serve URL schemes
    /// that the library can't load itself. The provider replaces the one set
    /// before.
    ///
    /// See `mln_runtime_set_resource_provider` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_provider(&self, provider: ResourceProvider) -> Result<()> {
        let mut call = self.0.inner.call();
        let provider = call.reference(&provider)?;
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_resource_provider(runtime, provider, completion, out_diagnostic)
        })
    }

    /// Rewrites the URLs of the runtime's network requests through a callback.
    ///
    /// The transform replaces the one set before.
    ///
    /// See `mln_runtime_set_resource_transform` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_resource_transform(&self, transform: ResourceTransform) -> Result<()> {
        let mut call = self.0.inner.call();
        let transform = call.reference(&transform)?;
        call.discard(|runtime, completion, out_diagnostic| unsafe {
            sys::mln_runtime_set_resource_transform(runtime, transform, completion, out_diagnostic)
        })
    }
}
