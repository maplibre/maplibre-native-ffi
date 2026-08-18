use std::fmt;
use std::sync::Arc;

use maplibre_core::AmbientCacheOperation;
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::events::{OfflineRegionDownloadState, OfflineRegionStatus, RuntimeEventBatch};
use crate::handle::{ConcurrentNativeHandle, closed_handle_error, out_handle};
use crate::resource::{HttpHeaderTransformState, ResourceProviderState, ResourceTransformState};
use crate::{HandleOperationError, ResourceProviderDecision, Result, RuntimeEventMask};
#[cfg(test)]
use crate::{LatLngBounds, MapHandle, MapOptions};

pub use maplibre_core::runtime::{OfflineRegionDefinition, OfflineRegionInfo, RuntimeOptions};
pub(crate) use maplibre_core::runtime::{
    OfflineRegionDefinitionNativeExt, RuntimeOptionsNativeExt,
};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ConcurrentNativeHandle<sys::mln_runtime>,
}
impl RuntimeState {
    fn new(native: sys::mln_runtime) -> Result<Self> {
        // SAFETY: native came from a successful typed creation take and its
        // registry/control state supports calls from any thread.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_runtime") }?;
        Ok(Self { handle })
    }

    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RuntimeHandle"))
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        let runtime = self.native()?;
        // SAFETY: runtime is live and native consumes it only on success.
        maplibre_core::check(unsafe { sys::mln_runtime_release(runtime) })?;
        self.handle.mark_closed();
        Ok(())
    }

    fn set_resource_provider<F>(&self, callback: F) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        let replacement = ResourceProviderState::new(callback);
        let descriptor = replacement.descriptor();
        self.install_resource_provider(replacement, descriptor)
    }

    /// Installs a provider descriptor whose callback is null, exercising the
    /// native install-failure rollback path that public callers cannot reach.
    #[cfg(test)]
    fn set_resource_provider_with_rejected_descriptor_for_testing<F>(
        &self,
        callback: F,
    ) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        let replacement = ResourceProviderState::new(callback);
        let mut descriptor = replacement.descriptor();
        descriptor.callback = None;
        self.install_resource_provider(replacement, descriptor)
    }

    fn install_resource_provider(
        &self,
        replacement: Box<ResourceProviderState>,
        descriptor: sys::mln_resource_provider,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let runtime = self.native()?;
        let replacement = Box::into_raw(replacement);
        // SAFETY: descriptor points to replacement, transferred before native can accept and
        // release it. A rejected registration leaves ownership with this binding.
        let result = completion::submit(
            |completion| unsafe {
                sys::mln_runtime_set_resource_provider(runtime, &descriptor, completion)
            },
            completion::command,
        );
        if result.is_err() {
            // SAFETY: native retains no callback state after rejected submission.
            drop(unsafe { Box::from_raw(replacement) });
        }
        result
    }

    fn clear_resource_provider(&self) -> Result<NativeFuture<CommandCompletion>> {
        let runtime = self.native()?;
        completion::submit(
            |completion| unsafe { sys::mln_runtime_clear_resource_provider(runtime, completion) },
            completion::command,
        )
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();
        let replacement = Box::into_raw(replacement);
        let result = completion::submit(
            |completion| unsafe {
                sys::mln_runtime_set_resource_transform(runtime, &descriptor, completion)
            },
            completion::command,
        );
        if result.is_err() {
            drop(unsafe { Box::from_raw(replacement) });
        }
        result
    }

    fn clear_resource_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        let runtime = self.native()?;
        completion::submit(
            |completion| unsafe { sys::mln_runtime_clear_resource_transform(runtime, completion) },
            completion::command,
        )
    }

    fn set_http_header_transform<F>(&self, callback: F) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        let runtime = self.native()?;
        let replacement = HttpHeaderTransformState::new(callback);
        let descriptor = replacement.descriptor();
        let replacement = Box::into_raw(replacement);
        let result = completion::submit(
            |completion| unsafe {
                sys::mln_runtime_set_http_header_transform(runtime, &descriptor, completion)
            },
            completion::command,
        );
        if result.is_err() {
            drop(unsafe { Box::from_raw(replacement) });
        }
        result
    }

    fn clear_http_header_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        let runtime = self.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_clear_http_header_transform(runtime, completion)
            },
            completion::command,
        )
    }
}

impl Drop for RuntimeState {
    fn drop(&mut self) {
        if self.close().is_err() {
            self.handle.leak_for_report();
        }
    }
}

/// Any-thread runtime handle backed by a core-owned worker.
pub struct RuntimeHandle {
    pub(crate) inner: Arc<RuntimeState>,
}

impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl RuntimeHandle {
    /// Creates a runtime on the current thread using explicit options.
    pub fn with_options(options: &RuntimeOptions) -> Result<Self> {
        maplibre_core::validate_abi_version()?;
        let native_options = options.to_native()?;
        let raw_options = native_options.to_raw();
        Self::create_with_native_options_after_abi_validation(&raw_options)
    }

    #[cfg(test)]
    fn create_with_native_options_after_abi_version_check_for_testing(
        options: *const sys::mln_runtime_options,
        actual_abi_version: u32,
    ) -> Result<Self> {
        maplibre_core::validate_abi_version_value(actual_abi_version)?;
        Self::create_with_native_options_after_abi_validation(options)
    }

    fn create_with_native_options_after_abi_validation(
        options: *const sys::mln_runtime_options,
    ) -> Result<Self> {
        // SAFETY: A nonnull options pointer comes from the binding's
        // materialized native options and remains readable for this call.
        let raw_options = if options.is_null() {
            unsafe { sys::mln_runtime_options_default() }
        } else {
            unsafe { *options }
        };
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_runtime>::new();
        // SAFETY: raw_options remains readable and out is null writable storage.
        maplibre_core::check(unsafe { sys::mln_runtime_create(&raw_options, out.as_mut_ptr()) })?;
        out_handle(out, "mln_runtime").and_then(|ptr| {
            Ok(Self {
                inner: Arc::new(RuntimeState::new(ptr)?),
            })
        })
    }

    /// Installs or replaces the runtime-scoped network resource provider.
    ///
    /// Native may invoke the closure from worker or network threads, so keep it
    /// quick and call no map or runtime APIs from it. Return `PassThrough` to
    /// let native networking handle the request, or `Handle` to complete or
    /// release the provided `ResourceRequestHandle` inline or later.
    ///
    /// A committed replacement retires the previous provider after its
    /// in-flight callbacks return and then releases its Rust state. Handles the
    /// previous provider already took stay valid; complete and release each one
    /// as usual.
    pub fn set_resource_provider<F>(&self, callback: F) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.inner.set_resource_provider(callback)
    }

    /// Clears the runtime-scoped network resource provider, sending later
    /// requests to MapLibre's online file source. The command commits after
    /// in-flight provider callbacks return and releases the Rust callback state.
    /// Handles the provider already took stay valid.
    pub fn clear_resource_provider(&self) -> Result<NativeFuture<CommandCompletion>> {
        self.inner.clear_resource_provider()
    }

    /// Installs or replaces the runtime-scoped network URL transform.
    ///
    /// Native may invoke the closure from worker or network threads, so keep it
    /// quick and call no MapLibre Native APIs from it. `Some(url)` replaces the
    /// request URL; `None`, an empty string, or a panic keeps the original.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform. The command commits
    /// after in-flight transform callbacks return and releases the Rust callback
    /// state.
    pub fn clear_resource_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        self.inner.clear_resource_transform()
    }

    /// Installs or replaces the runtime-scoped outgoing HTTP header transform.
    ///
    /// Native invokes the closure synchronously on worker or network threads
    /// after URL transformation. Returned headers are copied before the closure
    /// returns. Panics, duplicate names, and invalid headers leave the request
    /// unchanged.
    pub fn set_http_header_transform<F>(
        &self,
        callback: F,
    ) -> Result<NativeFuture<CommandCompletion>>
    where
        F: Fn(crate::HttpHeaderTransformRequest) -> Vec<crate::HttpHeader> + Send + Sync + 'static,
    {
        self.inner.set_http_header_transform(callback)
    }

    /// Clears the runtime-scoped outgoing HTTP header transform.
    pub fn clear_http_header_transform(&self) -> Result<NativeFuture<CommandCompletion>> {
        self.inner.clear_http_header_transform()
    }

    /// Starts an ambient cache maintenance operation for this runtime.
    pub fn ambient_cache_operation(
        &self,
        ambient_operation: AmbientCacheOperation,
    ) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_run_ambient_cache_operation(
                    runtime,
                    ambient_operation.to_native(),
                    completion,
                )
            },
            completion::unit,
        )
    }

    /// Starts a change to this runtime's maximum ambient cache size.
    ///
    /// MapLibre evicts ambient resources to fit the new budget, so lowering it
    /// discards cached resources. Offline regions are unaffected.
    pub fn set_maximum_ambient_cache_size(&self, size: u64) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_set_maximum_ambient_cache_size(runtime, size, completion)
            },
            completion::unit,
        )
    }

    /// Starts creating an offline region.
    pub fn create_offline_region(
        &self,
        definition: &OfflineRegionDefinition,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        let definition = definition.to_native()?;
        let raw_definition = definition.to_raw();
        // SAFETY: runtime is live. raw_definition points into definition-owned
        // string and geometry storage, metadata storage is valid for this call.
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_create(
                    runtime,
                    &raw_definition,
                    maplibre_core::runtime::metadata_ptr(metadata),
                    metadata.len(),
                    completion,
                )
            },
            copy_offline_region,
        )
    }

    /// Starts getting an offline region snapshot by ID.
    pub fn offline_region(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<Option<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live and operation points to writable storage.
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_get(runtime, region_id, completion)
            },
            |result| {
                if result.value_count == 0 {
                    return Ok(None);
                }
                copy_offline_region(result).map(Some)
            },
        )
    }

    /// Starts listing offline regions in this runtime's database.
    pub fn offline_regions(&self) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live and operation points to writable storage.
        completion::submit(
            |completion| unsafe { sys::mln_runtime_offline_regions_list(runtime, completion) },
            copy_offline_regions,
        )
    }

    /// Starts merging offline regions from another database path.
    pub fn merge_offline_regions_database(
        &self,
        path: &str,
    ) -> Result<NativeFuture<Vec<OfflineRegionInfo>>> {
        let runtime = self.inner.native()?;
        let path = maplibre_core::string::c_string(path)?;
        // SAFETY: runtime is live, path is NUL-terminated and valid for this
        // call, and operation points to writable storage.
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_regions_merge_database(runtime, path.as_ptr(), completion)
            },
            copy_offline_regions,
        )
    }

    /// Starts updating opaque metadata for an offline region.
    pub fn update_offline_region_metadata(
        &self,
        region_id: i64,
        metadata: &[u8],
    ) -> Result<NativeFuture<OfflineRegionInfo>> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live, metadata storage is valid for this call, and
        // operation points to writable storage.
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_update_metadata(
                    runtime,
                    region_id,
                    maplibre_core::runtime::metadata_ptr(metadata),
                    metadata.len(),
                    completion,
                )
            },
            copy_offline_region,
        )
    }

    /// Starts getting the current completed/download status for an offline region.
    pub fn offline_region_status(
        &self,
        region_id: i64,
    ) -> Result<NativeFuture<OfflineRegionStatus>> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live and operation points to writable storage.
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_get_status(runtime, region_id, completion)
            },
            |result| {
                Ok(maplibre_core::events::offline_region_status_from_native(
                    completion::copy_value(result)?,
                ))
            },
        )
    }

    /// Starts enabling or disabling runtime events for an offline region.
    pub fn set_offline_region_observed(
        &self,
        region_id: i64,
        observed: bool,
    ) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_set_observed(
                    runtime, region_id, observed, completion,
                )
            },
            completion::unit,
        )
    }

    /// Starts setting an offline region's native download state.
    pub fn set_offline_region_download_state(
        &self,
        region_id: i64,
        state: OfflineRegionDownloadState,
    ) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        let state = state.raw_for_set()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_set_download_state(
                    runtime, region_id, state, completion,
                )
            },
            completion::unit,
        )
    }

    /// Starts invalidating cached resources for an offline region.
    pub fn invalidate_offline_region(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_invalidate(runtime, region_id, completion)
            },
            completion::unit,
        )
    }

    /// Starts deleting an offline region.
    pub fn delete_offline_region(&self, region_id: i64) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe {
                sys::mln_runtime_offline_region_delete(runtime, region_id, completion)
            },
            completion::unit,
        )
    }

    /// Starts an ordered barrier that completes after all previously accepted
    /// runtime work reaches a terminal disposition.
    pub fn barrier(&self) -> Result<NativeFuture<()>> {
        let runtime = self.inner.native()?;
        completion::submit(
            |completion| unsafe { sys::mln_runtime_barrier(runtime, completion) },
            completion::unit,
        )
    }

    /// Drains an owned batch of queued runtime events.
    ///
    /// The returned batch contains the whole queue, remains stable across later
    /// drains, and releases its native storage when dropped.
    pub fn drain_events(&self) -> Result<RuntimeEventBatch> {
        let runtime = self.inner.native()?;
        let mut batch = sys::mln_event_batch(0);
        // SAFETY: runtime is live and batch points to a null writable handle.
        maplibre_core::check(unsafe { sys::mln_runtime_drain_events(runtime, &mut batch) })?;
        // SAFETY: A successful drain returns one owned native batch.
        unsafe { RuntimeEventBatch::new(batch) }
    }

    /// Selects which runtime-originated event types this runtime queues.
    ///
    /// A runtime reads the bits in
    /// [`RuntimeEventMask::ALL_RUNTIME_EVENTS`](crate::RuntimeEventMask::ALL_RUNTIME_EVENTS),
    /// so [`RuntimeEventMask::ALL`](crate::RuntimeEventMask::ALL) selects every
    /// runtime-originated type. Narrowing gates later events and keeps queued
    /// ones. A bit outside `ALL` is an invalid-argument error.
    pub fn set_event_mask(&self, mask: RuntimeEventMask) -> Result<()> {
        let runtime = self.inner.native()?;
        // SAFETY: runtime is live. The C API validates unknown mask bits.
        maplibre_core::check(unsafe { sys::mln_runtime_set_event_mask(runtime, mask.bits()) })
    }

    /// Reports which runtime-originated event types this runtime queues,
    /// starting from the mask its creation options selected.
    pub fn event_mask(&self) -> Result<RuntimeEventMask> {
        let runtime = self.inner.native()?;
        let mut raw = 0;
        // SAFETY: runtime is live and out_mask points to writable u64 storage.
        maplibre_core::check(unsafe { sys::mln_runtime_get_event_mask(runtime, &mut raw) })?;
        Ok(RuntimeEventMask::from_bits_retain(raw))
    }

    /// Explicitly releases the runtime's public handle.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.inner.is_closed() {
            return Ok(());
        }
        self.inner
            .close()
            .map_err(|error| HandleOperationError::new(error, self))
    }
}

fn copy_offline_region(result: &sys::mln_completion_result) -> Result<OfflineRegionInfo> {
    let raw = completion::copy_value::<sys::mln_offline_region_info>(result)?;
    maplibre_core::runtime::copy_offline_region_info(&raw)
}

fn copy_offline_regions(result: &sys::mln_completion_result) -> Result<Vec<OfflineRegionInfo>> {
    completion::copy_slice::<sys::mln_offline_region_info>(result)?
        .iter()
        .map(maplibre_core::runtime::copy_offline_region_info)
        .collect()
}

#[cfg(test)]
mod tests {
    // The fixture HTTP servers below are native-only; a browser build fetches
    // from the servers the test runner hosts instead.
    #[cfg(not(target_os = "emscripten"))]
    use std::io::{Read, Write};
    #[cfg(not(target_os = "emscripten"))]
    use std::net::TcpListener;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::{Arc, Mutex};
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use super::*;
    use crate::{
        Error, ErrorKind, ResourceErrorReason, ResourceKind, ResourceProviderDecision,
        ResourceResponse, RuntimeEvent, RuntimeEventSource, RuntimeEventType,
    };

    const PROVIDER_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;

    #[cfg(not(target_os = "emscripten"))]
    fn spawn_style_server(
        request_count: usize,
    ) -> (
        String,
        std::sync::mpsc::Receiver<String>,
        std::thread::JoinHandle<()>,
    ) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let base_url = format!("http://{}", listener.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();
        let handle = std::thread::spawn(move || {
            for _ in 0..request_count {
                let (mut stream, _) = listener.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut request = [0; 4096];
                let bytes = stream.read(&mut request).unwrap();
                let request = String::from_utf8_lossy(&request[..bytes]);
                let path = request
                    .lines()
                    .next()
                    .and_then(|line| line.split_whitespace().nth(1))
                    .unwrap_or("")
                    .to_owned();
                sender.send(path).unwrap();

                let body = PROVIDER_STYLE_JSON.as_bytes();
                write!(
                    stream,
                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                    body.len()
                )
                .unwrap();
                stream.write_all(body).unwrap();
            }
        });
        (base_url, receiver, handle)
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    fn spawn_recording_style_server(
        request_count: usize,
    ) -> (
        String,
        std::sync::mpsc::Receiver<String>,
        std::thread::JoinHandle<()>,
    ) {
        let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let base_url = format!("http://{}", listener.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();
        let handle = std::thread::spawn(move || {
            for _ in 0..request_count {
                let (mut stream, _) = listener.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut bytes = [0; 4096];
                let count = stream.read(&mut bytes).unwrap();
                sender
                    .send(String::from_utf8_lossy(&bytes[..count]).into_owned())
                    .unwrap();
                let body = PROVIDER_STYLE_JSON.as_bytes();
                write!(
                    stream,
                    "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                    body.len()
                )
                .unwrap();
                stream.write_all(body).unwrap();
            }
        });
        (base_url, receiver, handle)
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    fn spawn_redirect_style_servers() -> (
        String,
        std::sync::mpsc::Receiver<(String, bool)>,
        Vec<std::thread::JoinHandle<()>>,
    ) {
        let origin = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let destination = TcpListener::bind(("127.0.0.1", 0)).unwrap();
        let origin_url = format!("http://{}", origin.local_addr().unwrap());
        let destination_url = format!("http://{}", destination.local_addr().unwrap());
        let (sender, receiver) = std::sync::mpsc::channel();

        let origin_sender = sender.clone();
        let origin_destination_url = destination_url.clone();
        let origin_server = std::thread::spawn(move || {
            for _ in 0..3 {
                let (mut stream, _) = origin.accept().unwrap();
                stream
                    .set_read_timeout(Some(Duration::from_secs(5)))
                    .unwrap();
                let mut bytes = [0; 4096];
                let count = stream.read(&mut bytes).unwrap();
                let request = String::from_utf8_lossy(&bytes[..count]);
                let path = request
                    .lines()
                    .next()
                    .and_then(|line| line.split_whitespace().nth(1))
                    .unwrap_or("")
                    .to_owned();
                let has_header = request
                    .lines()
                    .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"));
                origin_sender.send((path.clone(), has_header)).unwrap();

                if path == "/same-start.json" {
                    write!(
                        stream,
                        "HTTP/1.1 302 Found\r\nLocation: /same-final.json\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                    )
                    .unwrap();
                } else if path == "/cross-start.json" {
                    write!(
                        stream,
                        "HTTP/1.1 302 Found\r\nLocation: {origin_destination_url}/cross-final.json\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                    )
                    .unwrap();
                } else {
                    let body = PROVIDER_STYLE_JSON.as_bytes();
                    write!(
                        stream,
                        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                        body.len()
                    )
                    .unwrap();
                    stream.write_all(body).unwrap();
                }
            }
        });

        let destination_server = std::thread::spawn(move || {
            let (mut stream, _) = destination.accept().unwrap();
            stream
                .set_read_timeout(Some(Duration::from_secs(5)))
                .unwrap();
            let mut bytes = [0; 4096];
            let count = stream.read(&mut bytes).unwrap();
            let request = String::from_utf8_lossy(&bytes[..count]);
            let path = request
                .lines()
                .next()
                .and_then(|line| line.split_whitespace().nth(1))
                .unwrap_or("")
                .to_owned();
            let has_header = request
                .lines()
                .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"));
            sender.send((path, has_header)).unwrap();
            let body = PROVIDER_STYLE_JSON.as_bytes();
            write!(
                stream,
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                body.len()
            )
            .unwrap();
            stream.write_all(body).unwrap();
        });

        (
            origin_url,
            receiver,
            vec![origin_server, destination_server],
        )
    }

    fn wait_for_operation<T>(
        _runtime: &mut RuntimeHandle,
        operation: &NativeFuture<T>,
    ) -> Result<()> {
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            if Instant::now() >= deadline {
                return Err(Error::new(
                    ErrorKind::InvalidState,
                    None,
                    "timed out waiting for native completion",
                ));
            }
            std::thread::sleep(std::time::Duration::from_millis(1));
            if operation.is_completed()? {
                let status = operation.terminal_status()?;
                if status != sys::MLN_STATUS_OK {
                    return Err(Error::from_status_and_diagnostic(
                        status,
                        operation.diagnostic()?,
                    ));
                }
                return Ok(());
            }
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    #[test]
    // Spec coverage: BND-084.
    fn runtime_ambient_cache_operations_use_real_c_abi() {
        let base = TempDir::new("maplibre-rust-ambient-cache");
        let cache = base.path().join("ambient.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();

        for operation in [
            AmbientCacheOperation::PackDatabase,
            AmbientCacheOperation::Invalidate,
            AmbientCacheOperation::Clear,
            AmbientCacheOperation::ResetDatabase,
        ] {
            let operation = runtime.ambient_cache_operation(operation).unwrap();
            wait_for_operation(&mut runtime, &operation).unwrap();
            operation.finish().unwrap();
            operation.release();
        }

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn runtime_set_maximum_ambient_cache_size_reports_completion() {
        let base = TempDir::new("maplibre-rust-cache-size");
        let cache = base.path().join("ambient-size.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();

        // Raising then lowering the budget exercises the same operation API.
        for size in [8 * 1024 * 1024, 0] {
            let operation = runtime.set_maximum_ambient_cache_size(size).unwrap();
            wait_for_operation(&mut runtime, &operation).unwrap();
            operation.finish().unwrap();
            operation.release();
        }

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn operation_remains_usable_after_runtime_close() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let runtime = RuntimeHandle::with_options(&options).unwrap();
        let operation = runtime
            .ambient_cache_operation(AmbientCacheOperation::Clear)
            .unwrap();

        runtime.close().unwrap();
        assert!(operation.wait(Duration::from_secs(10)).unwrap());
        operation.finish().unwrap();
    }

    #[test]
    // Spec coverage: BND-084 and BND-085.
    fn offline_region_apis_use_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let mut runtime = RuntimeHandle::with_options(&options).unwrap();
        let definition = test_offline_region_definition("custom://offline-style.json");

        let create = runtime.create_offline_region(&definition, b"abc").unwrap();
        wait_for_operation(&mut runtime, &create).unwrap();
        let created = create.take().unwrap();
        create.release();
        assert_eq!(created.definition, definition);
        assert_eq!(created.metadata, b"abc");

        let geometry_definition = OfflineRegionDefinition::GeometryRegion {
            style_url: "custom://offline-geometry-style.json".into(),
            geometry: br#"{"type":"Point","coordinates":[-122.5,37.5]}"#.to_vec(),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        };
        let create_geometry = runtime
            .create_offline_region(&geometry_definition, b"geo")
            .unwrap();
        wait_for_operation(&mut runtime, &create_geometry).unwrap();
        let geometry_region = create_geometry.take().unwrap();
        create_geometry.release();
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let get = runtime.offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &get).unwrap();
        let fetched = get.take().unwrap().unwrap();
        get.release();
        assert_eq!(fetched, created);

        let list = runtime.offline_regions().unwrap();
        wait_for_operation(&mut runtime, &list).unwrap();
        let listed = list.take().unwrap();
        list.release();
        assert!(listed.iter().any(|region| region.id == created.id));

        let update = runtime
            .update_offline_region_metadata(created.id, b"")
            .unwrap();
        wait_for_operation(&mut runtime, &update).unwrap();
        let updated = update.take().unwrap();
        update.release();
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status_operation = runtime.offline_region_status(created.id).unwrap();
        wait_for_operation(&mut runtime, &status_operation).unwrap();
        let status = status_operation.take().unwrap();
        status_operation.release();
        assert!(matches!(
            status.download_state,
            OfflineRegionDownloadState::Inactive | OfflineRegionDownloadState::Active
        ));

        let set_inactive = runtime
            .set_offline_region_download_state(created.id, OfflineRegionDownloadState::Inactive)
            .unwrap();
        wait_for_operation(&mut runtime, &set_inactive).unwrap();
        set_inactive.finish().unwrap();
        set_inactive.release();
        let error = runtime
            .set_offline_region_download_state(created.id, OfflineRegionDownloadState::Unknown(99))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        let observe = runtime
            .set_offline_region_observed(created.id, true)
            .unwrap();
        wait_for_operation(&mut runtime, &observe).unwrap();
        observe.finish().unwrap();
        observe.release();
        let unobserve = runtime
            .set_offline_region_observed(created.id, false)
            .unwrap();
        wait_for_operation(&mut runtime, &unobserve).unwrap();
        unobserve.finish().unwrap();
        unobserve.release();
        let invalidate = runtime.invalidate_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &invalidate).unwrap();
        invalidate.finish().unwrap();
        invalidate.release();
        let delete = runtime.delete_offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &delete).unwrap();
        delete.finish().unwrap();
        delete.release();
        let delete_geometry = runtime.delete_offline_region(geometry_region.id).unwrap();
        wait_for_operation(&mut runtime, &delete_geometry).unwrap();
        delete_geometry.finish().unwrap();
        delete_geometry.release();

        let missing_created = runtime.offline_region(created.id).unwrap();
        wait_for_operation(&mut runtime, &missing_created).unwrap();
        assert!(missing_created.take().unwrap().is_none());
        missing_created.release();
        let missing_geometry = runtime.offline_region(geometry_region.id).unwrap();
        wait_for_operation(&mut runtime, &missing_geometry).unwrap();
        assert!(missing_geometry.take().unwrap().is_none());
        missing_geometry.release();

        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-084.
    fn offline_region_merge_database_uses_real_c_abi() {
        let base = TempDir::new("maplibre-rust-offline-merge");
        let main_cache = base.path().join("main.db");
        let side_cache = base.path().join("side.db");

        let definition = test_offline_region_definition("custom://merge-style.json");
        {
            let mut side_options = RuntimeOptions::default();
            side_options.cache_path = Some(side_cache.to_string_lossy().into_owned());
            let mut side_runtime = RuntimeHandle::with_options(&side_options).unwrap();
            let create = side_runtime
                .create_offline_region(&definition, b"merge")
                .unwrap();
            wait_for_operation(&mut side_runtime, &create).unwrap();
            create.take().unwrap();
            create.release();
            side_runtime.close().unwrap();
        }

        let mut main_options = RuntimeOptions::default();
        main_options.cache_path = Some(main_cache.to_string_lossy().into_owned());
        let mut main_runtime = RuntimeHandle::with_options(&main_options).unwrap();
        let merge = main_runtime
            .merge_offline_regions_database(&side_cache.to_string_lossy())
            .unwrap();
        wait_for_operation(&mut main_runtime, &merge).unwrap();
        let merged = merge.take().unwrap();
        merge.release();
        assert_eq!(merged.len(), 1);
        assert_eq!(merged[0].definition, definition);
        assert_eq!(merged[0].metadata, b"merge");
        main_runtime.close().unwrap();
    }

    fn test_offline_region_definition(style_url: &str) -> OfflineRegionDefinition {
        OfflineRegionDefinition::TilePyramid {
            style_url: style_url.into(),
            bounds: LatLngBounds::new(
                crate::LatLng::new(37.0, -123.0),
                crate::LatLng::new(38.0, -122.0),
            ),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        }
    }

    struct TempDir {
        path: std::path::PathBuf,
    }

    impl TempDir {
        fn new(prefix: &str) -> Self {
            let nanos = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_nanos();
            let path =
                std::env::temp_dir().join(format!("{prefix}-{}-{nanos}", std::process::id()));
            std::fs::create_dir_all(&path).unwrap();
            Self { path }
        }

        fn path(&self) -> &std::path::Path {
            &self.path
        }
    }

    impl Drop for TempDir {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.path);
        }
    }

    #[test]
    // Spec coverage: BND-040.
    fn runtime_create_with_explicit_options_uses_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.asset_path = Some(String::new());
        options.cache_path = Some(String::new());
        let runtime = RuntimeHandle::with_options(&options).unwrap();

        std::thread::sleep(std::time::Duration::from_millis(1));
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-001.
    fn runtime_creation_rejects_abi_mismatch_before_storing_handle() {
        let error = RuntimeHandle::create_with_native_options_after_abi_version_check_for_testing(
            std::ptr::null(),
            maplibre_core::EXPECTED_C_ABI_VERSION + 1,
        )
        .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::AbiVersionMismatch);
        assert_eq!(error.raw_status(), None);
        assert!(
            error
                .diagnostic()
                .contains("unsupported MapLibre Native C ABI version")
        );
    }

    fn drain_holds_event_type(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        runtime
            .drain_events()
            .unwrap()
            .iter()
            .any(|event| event.event_type() == event_type)
    }

    fn wait_for_runtime_event(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..100 {
            std::thread::sleep(std::time::Duration::from_millis(1));
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        false
    }

    fn wait_for_arc_release(value: &Arc<()>) {
        let deadline = std::time::Instant::now() + Duration::from_secs(5);
        while Arc::strong_count(value) != 1 && std::time::Instant::now() < deadline {
            std::thread::yield_now();
        }
        assert_eq!(Arc::strong_count(value), 1);
    }

    fn wait_for_map_loading_failure(runtime: &mut RuntimeHandle) -> RuntimeEvent {
        for _ in 0..100 {
            std::thread::sleep(std::time::Duration::from_millis(1));
            let mut failure = None;
            for event in runtime.drain_events().unwrap().iter() {
                if event.event_type() == RuntimeEventType::MapLoadingFailed {
                    failure = Some(event.to_owned().unwrap());
                    break;
                }
            }
            if let Some(failure) = failure {
                return failure;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        panic!("expected a map loading-failure event");
    }

    #[test]
    // Spec coverage: BND-080.
    fn runtime_create_run_drain_and_close() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

        std::thread::sleep(std::time::Duration::from_millis(1));
        // A fresh runtime with no map queues nothing.
        let batch = runtime.drain_events().unwrap();
        assert!(batch.is_empty());
        assert_eq!(batch.len(), 0);
        assert_eq!(batch.iter().count(), 0);
        // A second drain of an empty queue reports the same empty batch.
        assert!(runtime.drain_events().unwrap().is_empty());
        runtime.close().unwrap();
    }

    fn wait_for_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..500 {
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            std::thread::sleep(Duration::from_millis(1));
        }
        false
    }

    #[test]
    fn native_worker_makes_progress_without_host_driving() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn runtime_accepts_concurrent_barrier_submissions() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let first = scope.spawn(|| runtime.barrier().unwrap());
            let second = scope.spawn(|| runtime.barrier().unwrap());
            for operation in [first.join().unwrap(), second.join().unwrap()] {
                assert!(operation.wait(Duration::from_secs(5)).unwrap());
                assert_eq!(operation.terminal_status().unwrap(), sys::MLN_STATUS_OK);
                operation.finish().unwrap();
            }
        });
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-060 and BND-091.
    fn a_creation_mask_is_applied_when_the_runtime_is_created() {
        let mut options = crate::RuntimeOptions::default();
        options.event_mask = RuntimeEventMask::MAP_STYLE_LOADED;
        let runtime = RuntimeHandle::with_options(&options).unwrap();

        assert_eq!(
            runtime.event_mask().unwrap(),
            RuntimeEventMask::MAP_STYLE_LOADED
        );
        runtime.close().unwrap();

        // A creation mask carrying an undefined bit fails before the runtime
        // exists.
        options.event_mask = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = RuntimeHandle::with_options(&options).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    }

    #[test]
    // Spec coverage: BND-091.
    fn a_runtime_mask_round_trips_and_rejects_undefined_bits() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

        // The default options mask selects every type.
        assert_eq!(runtime.event_mask().unwrap(), RuntimeEventMask::ALL);

        runtime.set_event_mask(RuntimeEventMask::ALL).unwrap();
        assert_eq!(runtime.event_mask().unwrap(), RuntimeEventMask::ALL);

        // Read, clear one bit, write back: every other bit survives.
        let mut mask = runtime.event_mask().unwrap();
        mask.remove(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED);
        runtime.set_event_mask(mask).unwrap();
        let read_back = runtime.event_mask().unwrap();
        assert!(!read_back.contains(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED));
        assert!(read_back.contains(RuntimeEventMask::MAP_IDLE));
        assert!(read_back.contains(RuntimeEventMask::MAP_STYLE_LOADED));

        let undefined = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = runtime.set_event_mask(undefined).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(runtime.event_mask().unwrap(), read_back);

        runtime.close().unwrap();
    }

    #[test]
    fn runtime_state_is_readable_from_another_thread() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let mask = scope
                .spawn(|| runtime.event_mask().unwrap())
                .join()
                .unwrap();
            assert_eq!(mask, RuntimeEventMask::ALL);
        });
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-123.
    fn resource_provider_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_provider().unwrap();
        assert_eq!(Arc::strong_count(&second), 2);

        let third = Arc::new(());
        let third_callback = Arc::clone(&third);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &third_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&third), 2);

        runtime.close().unwrap();
        wait_for_arc_release(&third);
        wait_for_arc_release(&first);
        wait_for_arc_release(&second);
    }

    #[test]
    // Spec coverage: BND-122.
    fn resource_provider_replacement_rolls_back_when_native_install_fails() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        let error = runtime
            .inner
            .set_resource_provider_with_rejected_descriptor_for_testing(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 1);

        runtime.close().unwrap();
        wait_for_arc_release(&first);
    }

    // Requests a style no file source serves, so the loading-failure event that
    // follows proves the request reached the network file source where the
    // runtime-scoped provider sits.
    fn load_probe_style(runtime: &mut RuntimeHandle, map: &MapHandle, style_url: &str) {
        map.set_style_url(style_url).unwrap();
        let event = wait_for_map_loading_failure(runtime);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| message.contains("\"jar\""))
        );
    }

    #[test]
    // Spec coverage: BND-142 and BND-154.
    fn resource_provider_is_consulted_until_replaced_and_cleared_while_a_map_is_live() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        let first_calls = Arc::new(AtomicUsize::new(0));
        let first_callback_calls = Arc::clone(&first_calls);
        runtime
            .set_resource_provider(move |_, _| {
                first_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            })
            .unwrap();
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/first.json");
        assert!(first_calls.load(Ordering::SeqCst) > 0);

        let second_calls = Arc::new(AtomicUsize::new(0));
        let second_callback_calls = Arc::clone(&second_calls);
        runtime
            .set_resource_provider(move |_, _| {
                second_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            })
            .unwrap();
        let first_calls_after_replace = first_calls.load(Ordering::SeqCst);
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/second.json");
        assert!(second_calls.load(Ordering::SeqCst) > 0);
        assert_eq!(
            first_calls.load(Ordering::SeqCst),
            first_calls_after_replace
        );

        runtime.clear_resource_provider().unwrap();
        let second_calls_after_clear = second_calls.load(Ordering::SeqCst);
        load_probe_style(&mut runtime, &map, "jar:file:/packaged/third.json");
        assert_eq!(
            first_calls.load(Ordering::SeqCst),
            first_calls_after_replace
        );
        assert_eq!(
            second_calls.load(Ordering::SeqCst),
            second_calls_after_clear
        );

        // Clearing an already cleared provider stays a successful no-op.
        runtime.clear_resource_provider().unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-143 and BND-150.
    fn resource_provider_completes_style_request_inline_through_c_abi() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url != "custom://style.json" {
                    return ResourceProviderDecision::PassThrough;
                }
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(ResourceResponse::ok(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::PassThrough
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url("custom://style.json").unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-155.
    fn resource_provider_sees_scheme_alias_and_its_resolved_url() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let resolved = Arc::new(Mutex::new(None));
        let callback_resolved = Arc::clone(&resolved);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url != "maplibre://maps/style" {
                    return ResourceProviderDecision::PassThrough;
                }
                *callback_resolved.lock().unwrap() = Some(request.resolved_url.clone());
                handle
                    .complete(ResourceResponse::ok(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::Handle
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url("maplibre://maps/style").unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            resolved.lock().unwrap().as_deref(),
            Some("https://demotiles.maplibre.org/style.json")
        );
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-144 and BND-145.
    fn resource_provider_completes_style_request_from_another_thread() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url == "custom://async-style.json" {
                    sender.send(handle).unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url("custom://async-style.json").unwrap();
        let handle = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should send handled request");
        assert!(!handle.is_cancelled().unwrap());
        std::thread::spawn(move || {
            handle
                .complete(ResourceResponse::ok(
                    PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                ))
                .unwrap();
        })
        .join()
        .unwrap();

        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-149.
    fn resource_provider_error_response_becomes_copied_loading_failure_event() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.requested_url == "custom://broken-style.json" {
                    handle
                        .complete(ResourceResponse::error(
                            ResourceErrorReason::Other,
                            "provider failed",
                        ))
                        .unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        let map_id = map.id();
        map.set_style_url("custom://broken-style.json").unwrap();

        let event = wait_for_map_loading_failure(&mut runtime);
        let copied_message = event.message.clone();
        // The copy stays intact after the drain that ends the batch's window.
        let _ = runtime.drain_events().unwrap();

        assert_eq!(event.source, RuntimeEventSource::Map(map_id));
        assert_eq!(event.event_type, RuntimeEventType::MapLoadingFailed);
        assert_eq!(event.message, copied_message);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| message.contains("provider failed"))
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-140 and BND-123.
    fn resource_transform_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        runtime
            .set_resource_transform(move |request| {
                let _ = &first_callback;
                assert!(matches!(
                    request.kind,
                    ResourceKind::Style | ResourceKind::UnknownRaw(_)
                ));
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                Some("https://example.test/replacement".to_owned())
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_transform().unwrap();
        assert_eq!(Arc::strong_count(&second), 2);
        runtime.close().unwrap();
        wait_for_arc_release(&first);
        wait_for_arc_release(&second);
    }

    /// The browser has no in-process TCP server, so the runner serves the two
    /// style documents and answers 404 for everything else. That makes the
    /// server the oracle: a rewrite that did not happen fails the style load
    /// rather than fetching an identical document, and the layer each document
    /// carries names which one was used. See scripts/run-browser-test.mjs.
    #[cfg(target_os = "emscripten")]
    #[test]
    // Spec coverage: BND-140.
    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let origin = std::env::var("MLN_FFI_TEST_FIXTURE_ORIGIN").expect(
            "MLN_FFI_TEST_FIXTURE_ORIGIN is unset; run the suite through \
             `mise run //bindings/rust:test emscripten-wasm32-webgl`",
        );
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let transform_url = format!("{origin}/__fixture/rewritten-style.json");
        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the layer id says so.
        runtime
            .set_resource_transform(move |request| {
                (request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json"))
                .then(|| transform_url.clone())
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url(&format!("{origin}/__fixture/original-style.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        // Contains rather than equals: MapLibre adds its own annotation layer to
        // every style it loads.
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(layer_ids.take().unwrap().iter().any(|id| id == "rewritten"));
        layer_ids.release();

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{origin}/__fixture/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let layer_ids = map.style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(
            layer_ids
                .take()
                .unwrap()
                .iter()
                .any(|id| id == "original-after-clear")
        );
        layer_ids.release();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[cfg(not(target_os = "emscripten"))]
    #[test]
    // Spec coverage: BND-140.
    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_style_server(2);
        let transform_base_url = base_url.clone();

        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the recorded path
        // says so.
        runtime
            .set_resource_transform(move |request| {
                if request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json")
                {
                    Some(format!("{transform_base_url}/rewritten-style.json"))
                } else {
                    None
                }
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url(&format!("{base_url}/original-style.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/rewritten-style.json"
        );

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{base_url}/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/original-after-clear.json"
        );

        map.close().unwrap();
        runtime.close().unwrap();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-158 and BND-159.
    fn http_header_transform_reaches_requests_and_clear_stops_it() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_recording_style_server(2);
        runtime
            .set_http_header_transform(|request| {
                assert_eq!(request.kind, ResourceKind::Style);
                vec![crate::HttpHeader::new("X-Map-Token", "secret")]
            })
            .unwrap();

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.set_style_url(&format!("{base_url}/with-header.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let first = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            first
                .lines()
                .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"))
        );

        runtime.clear_http_header_transform().unwrap();
        map.set_style_url(&format!("{base_url}/after-clear.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        let second = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            !second
                .lines()
                .any(|line| line.to_ascii_lowercase().starts_with("x-map-token:"))
        );

        map.close().unwrap();
        runtime.close().unwrap();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-158.
    fn http_header_transform_skips_non_http_urls() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        runtime.set_resource_transform(|_| None).unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_http_header_transform(move |_| {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                Vec::new()
            })
            .unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        map.set_style_url("jar:file:/packaged/style.json").unwrap();
        let _ = wait_for_map_loading_failure(&mut runtime);
        assert_eq!(calls.load(Ordering::SeqCst), 0);

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]
    // Spec coverage: BND-159.
    fn http_header_transform_preserves_same_origin_and_strips_cross_origin_redirects() {
        let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let (origin_url, requests, servers) = spawn_redirect_style_servers();
        runtime
            .set_http_header_transform(|_| vec![crate::HttpHeader::new("X-Map-Token", "secret")])
            .unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        map.set_style_url(&format!("{origin_url}/same-start.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/same-start.json".to_owned(), true)
        );
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/same-final.json".to_owned(), true)
        );

        map.set_style_url(&format!("{origin_url}/cross-start.json"))
            .unwrap();
        assert!(wait_for_runtime_event(
            &mut runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-start.json".to_owned(), true)
        );
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-final.json".to_owned(), false)
        );

        map.close().unwrap();
        runtime.close().unwrap();
        for server in servers {
            server.join().unwrap();
        }
    }

    // OpenHarmony's platform HTTP client has no redirect-decision hook, and the
    // browser's fetch transport follows redirects itself, so both report header
    // transforms unsupported rather than enabling a transport that cannot
    // satisfy the redirect contract.
    #[cfg(any(target_env = "ohos", target_os = "emscripten"))]
    #[test]
    fn http_header_transform_reports_unsupported() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let error = runtime
            .set_http_header_transform(|_| Vec::new())
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::Unsupported);
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-123.
    fn resource_transform_replacement_after_map_creation_releases_previous_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_transform(move |_| {
                let _ = &first_callback;
                None
            })
            .unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                None
            })
            .unwrap();

        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 2);

        map.close().unwrap();
        runtime.close().unwrap();
        wait_for_arc_release(&second);
        wait_for_arc_release(&first);
    }

    #[test]
    // Spec coverage: BND-123.
    fn runtime_teardown_releases_resource_transform_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();

        wait_for_arc_release(&token);
    }

    #[test]
    // Rust regression: documents the Rust binding's late transform-install
    // guard after map creation.
    fn resource_transform_installs_after_map_creation() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        runtime.set_resource_transform(|_| None).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-140 and BND-123.
    fn resource_transform_clears_after_map_was_closed_and_releases_state() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        map.close().unwrap();

        runtime.clear_resource_transform().unwrap();

        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();
        wait_for_arc_release(&token);
    }

    #[test]
    // Spec coverage: BND-081, BND-090, and BND-092.
    fn a_drain_reports_map_events_in_queue_order_and_copies_outlive_the_batch() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
        let map_id = map.id();

        let command = map.set_style_json(b"{").unwrap();
        assert!(command.wait(Duration::from_secs(5)).unwrap());
        let completion = command.take().unwrap();
        assert_eq!(completion.disposition, crate::CommandDisposition::Failed);
        assert_eq!(completion.raw_status, sys::MLN_STATUS_NATIVE_ERROR);
        assert!(!completion.diagnostic.is_empty());

        let batch = runtime.drain_events().unwrap();
        let types = batch
            .iter()
            .map(|event| event.event_type())
            .collect::<Vec<_>>();
        assert!(
            !types.is_empty(),
            "a failed style load should queue an event, got {types:?}"
        );
        let loading_failed = batch
            .iter()
            .find(|event| event.event_type() == RuntimeEventType::MapLoadingFailed)
            .expect("a malformed style should queue a loading-failed event");
        assert_eq!(loading_failed.source(), RuntimeEventSource::Map(map_id));
        assert!(
            loading_failed
                .message()
                .unwrap()
                .is_some_and(|message| !message.is_empty())
        );
        let owned = loading_failed.to_owned().unwrap();

        // A later drain leaves the owned batch readable.
        assert!(runtime.drain_events().unwrap().is_empty());
        assert_eq!(loading_failed.source(), RuntimeEventSource::Map(map_id));
        drop(batch);
        assert_eq!(owned.source, RuntimeEventSource::Map(map_id));
        assert_eq!(owned.event_type, RuntimeEventType::MapLoadingFailed);
        assert!(
            owned
                .message
                .as_deref()
                .is_some_and(|message| !message.is_empty())
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    // Spec coverage: BND-042.
    fn runtime_close_with_live_map_is_rust_invalid_state_and_retryable() {
        let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
        let map =
            crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

        let error = runtime.close().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_STATE));
        let runtime = error.into_handle();

        std::thread::sleep(std::time::Duration::from_millis(1));
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
