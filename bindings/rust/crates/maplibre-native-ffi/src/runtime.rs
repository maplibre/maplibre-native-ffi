#[allow(clippy::all, unused_parens)]
mod generated;
use crate::handle::{ConcurrentNativeHandle, closed_handle_error};
use crate::{NativeFuture, Result};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;
use std::fmt;
use std::sync::Arc;

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ConcurrentNativeHandle<sys::mln_runtime>,
}
impl RuntimeState {
    fn new(native: sys::mln_runtime) -> Result<Self> {
        Ok(Self {
            handle: unsafe { ConcurrentNativeHandle::from_handle(native, "mln_runtime") }?,
        })
    }
    pub(crate) fn native(&self) -> Result<sys::mln_runtime> {
        maplibre_core::callback::check("", 0)?;
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RuntimeHandle"))
    }
}
impl Drop for RuntimeState {
    fn drop(&mut self) {
        self.handle
            .finalize_with(|handle| unsafe { maplibre_core::generated::runtime_dispose(handle) });
    }
}
/// Any-thread runtime handle backed by a native worker.
pub struct RuntimeHandle {
    pub(crate) inner: Arc<RuntimeState>,
}
impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.handle.is_closed())
            .finish()
    }
}
impl RuntimeHandle {
    pub(crate) fn from_native(native: sys::mln_runtime) -> Result<Self> {
        Ok(Self {
            inner: Arc::new(RuntimeState::new(native)?),
        })
    }
    #[cfg(test)]
    fn create_after_abi_version_check_for_testing(actual_abi_version: u32) -> Result<Self> {
        maplibre_core::validate_abi_version_value(actual_abi_version)?;
        crate::runtime_create(&crate::RuntimeOptions::default())
    }
    #[cfg(test)]
    pub(crate) fn close_and_wait(self) {
        crate::completion::blocking(self.release());
    }
}

#[cfg(test)]
mod tests {
    use crate::*;
    struct TransformRequest {
        kind: crate::ResourceKind,
        url: String,
    }
    fn resource_transform(
        callback: impl Fn(TransformRequest) -> Option<String> + Send + Sync + 'static,
    ) -> crate::ResourceTransform {
        crate::ResourceTransform::new(move |kind, url, response| {
            if let Some(url) = callback(TransformRequest { kind, url }) {
                response.set_url(&url)?;
            }
            Ok(())
        })
    }
    fn http_header_transform(
        callback: impl Fn(TransformRequest) -> Vec<(String, String)> + Send + Sync + 'static,
    ) -> crate::HttpHeaderTransform {
        crate::HttpHeaderTransform::new(move |kind, url, response| {
            for (name, value) in callback(TransformRequest { kind, url }) {
                response.set(&name, &value)?;
            }
            Ok(())
        })
    }

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
        ErrorKind, ResourceErrorReason, ResourceKind, ResourceProviderDecision, RuntimeEvent,
        RuntimeEventType,
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

    #[test]

    fn runtime_ambient_cache_operations_use_real_c_abi() {
        let base = TempDir::new("maplibre-rust-ambient-cache");
        let cache = base.path().join("ambient.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let runtime = crate::runtime_create(&options).unwrap();

        for operation in [
            AmbientCacheOperation::PackDatabase,
            AmbientCacheOperation::Invalidate,
            AmbientCacheOperation::Clear,
            AmbientCacheOperation::ResetDatabase,
        ] {
            crate::completion::blocking(runtime.run_ambient_cache_operation(operation));
        }

        runtime.close_and_wait();
    }

    #[test]

    fn runtime_set_maximum_ambient_cache_size_reports_completion() {
        let base = TempDir::new("maplibre-rust-cache-size");
        let cache = base.path().join("ambient-size.db");

        let mut options = RuntimeOptions::default();
        options.cache_path = Some(cache.to_string_lossy().into_owned());
        let runtime = crate::runtime_create(&options).unwrap();

        // Raising then lowering the budget exercises the same operation API.
        for size in [8 * 1024 * 1024, 0] {
            crate::completion::blocking(runtime.set_maximum_ambient_cache_size(size));
        }

        runtime.close_and_wait();
    }

    #[test]

    fn operation_remains_usable_after_runtime_close() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let runtime = crate::runtime_create(&options).unwrap();
        let operation = runtime
            .run_ambient_cache_operation(AmbientCacheOperation::Clear)
            .unwrap();

        runtime.close_and_wait();
        assert!(operation.wait(Duration::from_secs(10)).unwrap());
        operation.take().unwrap();
    }

    #[test]

    fn offline_region_apis_use_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.cache_path = Some(":memory:".into());
        let runtime = crate::runtime_create(&options).unwrap();
        let definition = test_offline_region_definition("custom://offline-style.json");

        let created =
            crate::completion::blocking(runtime.offline_region_create(&definition, b"abc"));
        assert_eq!(created.definition, definition);
        assert_eq!(created.metadata, b"abc");

        let geometry_definition =
            OfflineRegionDefinition::geometry(crate::OfflineGeometryRegionDefinition {
                style_url: "custom://offline-geometry-style.json".into(),
                geometry: br#"{"type":"Point","coordinates":[-122.5,37.5]}"#.to_vec(),
                min_zoom: 0.0,
                max_zoom: 1.0,
                pixel_ratio: 1.0,
                include_ideographs: false,
            });
        let geometry_region = crate::completion::blocking(
            runtime.offline_region_create(&geometry_definition, b"geo"),
        );
        assert_eq!(geometry_region.definition, geometry_definition);
        assert_eq!(geometry_region.metadata, b"geo");

        let fetched = crate::completion::blocking(runtime.offline_region_get(created.id)).unwrap();
        assert_eq!(fetched, created);

        let listed = crate::completion::blocking(runtime.offline_regions_list());
        assert!(listed.iter().any(|region| region.id == created.id));

        let updated =
            crate::completion::blocking(runtime.offline_region_update_metadata(created.id, b""));
        assert_eq!(updated.id, created.id);
        assert!(updated.metadata.is_empty());

        let status = crate::completion::blocking(runtime.offline_region_get_status(created.id));
        assert!(matches!(
            status.download_state,
            OfflineRegionDownloadState::Inactive | OfflineRegionDownloadState::Active
        ));

        crate::completion::blocking(
            runtime.offline_region_set_download_state(
                created.id,
                OfflineRegionDownloadState::Inactive,
            ),
        );
        let error = runtime
            .offline_region_set_download_state(created.id, OfflineRegionDownloadState::Unknown(99))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);

        crate::completion::blocking(runtime.offline_region_set_observed(created.id, true));
        crate::completion::blocking(runtime.offline_region_set_observed(created.id, false));
        crate::completion::blocking(runtime.offline_region_invalidate(created.id));
        crate::completion::blocking(runtime.offline_region_delete(created.id));
        crate::completion::blocking(runtime.offline_region_delete(geometry_region.id));

        assert!(crate::completion::blocking(runtime.offline_region_get(created.id)).is_none());
        assert!(
            crate::completion::blocking(runtime.offline_region_get(geometry_region.id)).is_none()
        );

        runtime.close_and_wait();
    }

    #[test]

    fn offline_region_merge_database_accepts_read_only_source_through_real_c_abi() {
        let base = TempDir::new("maplibre-rust-offline-merge");
        let main_cache = base.path().join("main.db");
        let side_cache = base.path().join("side.db");

        let definition = test_offline_region_definition("custom://merge-style.json");
        {
            let mut side_options = RuntimeOptions::default();
            side_options.cache_path = Some(side_cache.to_string_lossy().into_owned());
            let side_runtime = crate::runtime_create(&side_options).unwrap();
            crate::completion::blocking(side_runtime.offline_region_create(&definition, b"merge"));
            side_runtime.close_and_wait();
        }
        let side_database_before = std::fs::read(&side_cache).unwrap();

        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;

            let mut permissions = std::fs::metadata(&side_cache).unwrap().permissions();
            permissions.set_mode(0o444);
            std::fs::set_permissions(&side_cache, permissions).unwrap();
        }

        let mut main_options = RuntimeOptions::default();
        main_options.cache_path = Some(main_cache.to_string_lossy().into_owned());
        let main_runtime = crate::runtime_create(&main_options).unwrap();
        let merged = crate::completion::blocking(
            main_runtime.offline_regions_merge_database(&side_cache.to_string_lossy()),
        );
        assert_eq!(merged.len(), 1);
        assert_eq!(merged[0].definition, definition);
        assert_eq!(merged[0].metadata, b"merge");
        assert_eq!(std::fs::read(&side_cache).unwrap(), side_database_before);
        main_runtime.close_and_wait();
    }

    fn test_offline_region_definition(style_url: &str) -> OfflineRegionDefinition {
        OfflineRegionDefinition::tile_pyramid(crate::OfflineTilePyramidRegionDefinition {
            style_url: style_url.into(),
            bounds: LatLngBounds::new(
                crate::LatLng::new(37.0, -123.0),
                crate::LatLng::new(38.0, -122.0),
            ),
            min_zoom: 0.0,
            max_zoom: 1.0,
            pixel_ratio: 1.0,
            include_ideographs: false,
        })
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

    fn runtime_create_with_explicit_options_uses_real_c_abi() {
        let mut options = RuntimeOptions::default();
        options.asset_path = Some(String::new());
        options.cache_path = Some(String::new());
        let runtime = crate::runtime_create(&options).unwrap();

        // The barrier proves the worker ran with these options before close.
        crate::completion::blocking(runtime.barrier());
        runtime.close_and_wait();
    }

    #[test]

    fn runtime_creation_rejects_abi_mismatch_before_storing_handle() {
        let error = RuntimeHandle::create_after_abi_version_check_for_testing(
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
            .get()
            .unwrap()
            .events
            .iter()
            .any(|event| event.r#type == event_type)
    }

    /// Drains until one event of `event_type` arrives or the deadline passes.
    fn wait_for_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        let deadline = Instant::now() + Duration::from_secs(5);
        loop {
            if drain_holds_event_type(runtime, event_type) {
                return true;
            }
            if Instant::now() >= deadline {
                return false;
            }
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    fn wait_for_arc_release(value: &Arc<()>) {
        let deadline = std::time::Instant::now() + Duration::from_secs(5);
        while Arc::strong_count(value) != 1 && std::time::Instant::now() < deadline {
            std::thread::yield_now();
        }
        assert_eq!(Arc::strong_count(value), 1);
    }

    /// Drains until the map reports a loading failure.
    fn wait_for_map_loading_failure(runtime: &RuntimeHandle) -> RuntimeEvent {
        let deadline = Instant::now() + Duration::from_secs(5);
        let mut observed = Vec::new();
        loop {
            let batch = runtime.drain_events().unwrap();
            let events = batch.get().unwrap().events;
            for event in events {
                if event.r#type == RuntimeEventType::MapLoadingFailed {
                    return event;
                }
                observed.push(event);
            }
            assert!(
                Instant::now() < deadline,
                "expected a map loading-failure event; observed: {observed:?}"
            );
            std::thread::sleep(Duration::from_millis(1));
        }
    }

    #[test]

    fn runtime_create_run_drain_and_close() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();

        // The barrier fences the empty-queue claim: every submission the
        // runtime accepted so far has reached a terminal disposition.
        crate::completion::blocking(runtime.barrier());
        // A fresh runtime with no map queues nothing.
        let events = runtime.drain_events().unwrap().get().unwrap().events;
        assert!(events.is_empty());
        // A second drain of an empty queue reports the same empty sequence.
        assert!(
            runtime
                .drain_events()
                .unwrap()
                .get()
                .unwrap()
                .events
                .is_empty()
        );
        runtime.close_and_wait();
    }

    #[test]
    fn discarded_creation_future_retires_its_map() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let future = runtime.map_create(&MapOptions::default()).unwrap();
        crate::completion::blocking(runtime.barrier());
        drop(future);
        runtime.close_and_wait();
    }

    #[test]
    fn native_worker_makes_progress_without_host_driving() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn drained_events_can_be_read_from_another_thread() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_json(PROVIDER_STYLE_JSON.as_bytes()).unwrap();
        let deadline = Instant::now() + Duration::from_secs(5);
        let events = loop {
            let events = runtime.drain_events().unwrap().get().unwrap().events;
            if events
                .iter()
                .any(|event| event.r#type == RuntimeEventType::MapStyleLoaded)
            {
                break events;
            }
            assert!(Instant::now() < deadline, "the style never loaded");
            std::thread::sleep(Duration::from_millis(1));
        };

        // The drained events own their data, so they may be moved to whichever
        // thread reads them.
        let types =
            std::thread::spawn(move || events.iter().map(|event| event.r#type).collect::<Vec<_>>())
                .join()
                .unwrap();
        assert!(types.contains(&RuntimeEventType::MapStyleLoaded));

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]
    fn runtime_accepts_concurrent_barrier_submissions() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let first = scope.spawn(|| runtime.barrier().unwrap());
            let second = scope.spawn(|| runtime.barrier().unwrap());
            for operation in [first.join().unwrap(), second.join().unwrap()] {
                assert!(operation.wait(Duration::from_secs(5)).unwrap());
                operation.take().unwrap();
            }
        });
        runtime.close_and_wait();
    }

    #[test]

    fn a_creation_mask_is_applied_when_the_runtime_is_created() {
        let mut options = crate::RuntimeOptions::default();
        options.event_mask = RuntimeEventMask::MAP_STYLE_LOADED;
        let runtime = crate::runtime_create(&options).unwrap();

        assert_eq!(
            runtime.get_event_mask().unwrap(),
            RuntimeEventMask::MAP_STYLE_LOADED
        );
        runtime.close_and_wait();

        // A creation mask carrying an undefined bit fails before the runtime
        // exists.
        options.event_mask = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = crate::runtime_create(&options).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    }

    #[test]

    fn a_runtime_mask_round_trips_and_rejects_undefined_bits() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();

        // The default options mask selects every type.
        assert_eq!(runtime.get_event_mask().unwrap(), RuntimeEventMask::ALL);

        runtime.set_event_mask(RuntimeEventMask::ALL).unwrap();
        assert_eq!(runtime.get_event_mask().unwrap(), RuntimeEventMask::ALL);

        // Read, clear one bit, write back: every other bit survives.
        let mut mask = runtime.get_event_mask().unwrap();
        mask.remove(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED);
        runtime.set_event_mask(mask).unwrap();
        let read_back = runtime.get_event_mask().unwrap();
        assert!(!read_back.contains(RuntimeEventMask::OFFLINE_REGION_STATUS_CHANGED));
        assert!(read_back.contains(RuntimeEventMask::MAP_IDLE));
        assert!(read_back.contains(RuntimeEventMask::MAP_STYLE_LOADED));

        let undefined = RuntimeEventMask::from_bits_retain(1 << 63);
        let error = runtime.set_event_mask(undefined).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(runtime.get_event_mask().unwrap(), read_back);

        runtime.close_and_wait();
    }

    #[test]
    fn runtime_state_is_readable_from_another_thread() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        std::thread::scope(|scope| {
            let mask = scope
                .spawn(|| runtime.get_event_mask().unwrap())
                .join()
                .unwrap();
            assert_eq!(mask, RuntimeEventMask::ALL);
        });
        runtime.close_and_wait();
    }

    #[test]

    fn resource_provider_installs_replaces_clears_and_releases_state() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        completion::blocking(runtime.set_resource_provider(crate::ResourceProvider::new(
            move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            },
        )));
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        completion::blocking(runtime.set_resource_provider(crate::ResourceProvider::new(
            move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            },
        )));
        wait_for_arc_release(&first);
        assert_eq!(Arc::strong_count(&second), 2);

        completion::blocking(runtime.clear_resource_provider());
        wait_for_arc_release(&second);

        let third = Arc::new(());
        let third_callback = Arc::clone(&third);
        completion::blocking(runtime.set_resource_provider(crate::ResourceProvider::new(
            move |_, _| {
                let _ = &third_callback;
                crate::ResourceProviderDecision::PassThrough
            },
        )));
        assert_eq!(Arc::strong_count(&third), 2);

        runtime.close_and_wait();
        wait_for_arc_release(&third);
    }

    #[test]

    fn resource_provider_replacement_rolls_back_when_native_install_fails() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            }))
            .unwrap();

        let error = runtime
            .set_resource_provider(crate::ResourceProvider::default())
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert_eq!(Arc::strong_count(&first), 2);

        runtime.close_and_wait();
        wait_for_arc_release(&first);
    }

    // Requests a style no file source serves, so the loading-failure event that
    // follows proves the request reached the network file source where the
    // runtime-scoped provider sits.
    fn load_probe_style(runtime: &RuntimeHandle, map: &MapHandle, style_url: &str) {
        map.set_style_url(style_url).unwrap();
        let event = wait_for_map_loading_failure(runtime);
        assert!(event.message.contains("\"jar\""));
    }

    #[test]

    fn resource_provider_is_consulted_until_replaced_and_cleared_while_a_map_is_live() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        let first_calls = Arc::new(AtomicUsize::new(0));
        let first_callback_calls = Arc::clone(&first_calls);
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |_, handle| {
                drop(handle);
                let (sender, receiver) = std::sync::mpsc::channel();
                maplibre_core::callback::finalize(move || sender.send(()).unwrap());
                receiver.recv_timeout(Duration::from_secs(5)).expect(
                    "implicit request drop must finish without claiming the provider decision",
                );
                first_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            }))
            .unwrap();
        load_probe_style(&runtime, &map, "jar:file:/packaged/first.json");
        assert!(first_calls.load(Ordering::SeqCst) > 0);

        let second_calls = Arc::new(AtomicUsize::new(0));
        let second_callback_calls = Arc::clone(&second_calls);
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |_, _| {
                second_callback_calls.fetch_add(1, Ordering::SeqCst);
                ResourceProviderDecision::PassThrough
            }))
            .unwrap();
        let first_calls_after_replace = first_calls.load(Ordering::SeqCst);
        load_probe_style(&runtime, &map, "jar:file:/packaged/second.json");
        assert!(second_calls.load(Ordering::SeqCst) > 0);
        assert_eq!(
            first_calls.load(Ordering::SeqCst),
            first_calls_after_replace
        );

        runtime.clear_resource_provider().unwrap();
        let second_calls_after_clear = second_calls.load(Ordering::SeqCst);
        load_probe_style(&runtime, &map, "jar:file:/packaged/third.json");
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

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_provider_completes_style_request_inline_through_c_abi() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        let callback_runtime = Arc::downgrade(&runtime.inner);
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() != Some("custom://style.json") {
                    return ResourceProviderDecision::PassThrough;
                }
                let runtime = RuntimeHandle {
                    inner: callback_runtime.upgrade().unwrap(),
                };
                assert_eq!(
                    runtime.barrier().unwrap_err().kind(),
                    ErrorKind::InvalidState
                );
                assert_eq!(
                    runtime.release().unwrap_err().kind(),
                    ErrorKind::InvalidState
                );
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(&crate::test_support::ok_response(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::PassThrough
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://style.json").unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_provider_sees_scheme_alias_and_its_resolved_url() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let resolved = Arc::new(Mutex::new(None));
        let callback_resolved = Arc::clone(&resolved);
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() != Some("maplibre://maps/style") {
                    return ResourceProviderDecision::PassThrough;
                }
                *callback_resolved.lock().unwrap() = request.resolved_url.clone();
                handle
                    .complete(&crate::test_support::ok_response(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                ResourceProviderDecision::Handle
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("maplibre://maps/style").unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        assert_eq!(
            resolved.lock().unwrap().as_deref(),
            Some("https://demotiles.maplibre.org/style.json")
        );
        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_provider_completes_style_request_from_another_thread() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() == Some("custom://async-style.json") {
                    sender.send(handle).unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://async-style.json").unwrap();
        let handle = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should send handled request");
        assert!(!handle.cancelled().unwrap());
        std::thread::spawn(move || {
            handle
                .complete(&crate::test_support::ok_response(
                    PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                ))
                .unwrap();
        })
        .join()
        .unwrap();

        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        map.close_and_wait();
        runtime.close_and_wait();
    }

    /// Waits for the condition to hold, returning whether it did.
    fn wait_for_condition(condition: impl Fn() -> bool) -> bool {
        let deadline = Instant::now() + Duration::from_secs(5);
        while !condition() && Instant::now() < deadline {
            std::thread::sleep(Duration::from_millis(10));
        }
        condition()
    }

    /// Installs a resource provider and waits for the runtime to commit it, so
    /// the map created afterward sees it.
    fn commit_resource_provider<F>(runtime: &RuntimeHandle, callback: F)
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        crate::completion::blocking(
            runtime.set_resource_provider(crate::ResourceProvider::new(callback)),
        );
    }

    #[test]

    fn cancel_callback_runs_once_when_the_map_discards_the_request() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let cancels = Arc::new(AtomicUsize::new(0));
        let callback_cancels = Arc::clone(&cancels);
        let (sender, receiver) = std::sync::mpsc::channel();
        commit_resource_provider(&runtime, move |request, handle| {
            if request.requested_url.as_deref() != Some("custom://cancel-style.json") {
                return ResourceProviderDecision::PassThrough;
            }
            let cancels = Arc::clone(&callback_cancels);
            handle
                .set_cancel_callback(move || {
                    cancels.fetch_add(1, Ordering::SeqCst);
                })
                .unwrap();
            sender.send(handle).unwrap();
            ResourceProviderDecision::Handle
        });

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://cancel-style.json").unwrap();
        let handle = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should send handled request");
        assert_eq!(cancels.load(Ordering::SeqCst), 0);

        map.close_and_wait();

        assert!(wait_for_condition(|| cancels.load(Ordering::SeqCst) == 1));
        assert!(handle.cancelled().unwrap());
        // The cancelled request rejects a late completion and stays at one call.
        assert_eq!(
            handle
                .complete(&crate::ResourceResponse {
                    status: crate::ResourceResponseStatus::NoContent,
                    ..Default::default()
                })
                .unwrap_err()
                .kind(),
            ErrorKind::InvalidState
        );
        assert_eq!(cancels.load(Ordering::SeqCst), 1);
        handle.close().unwrap();
        runtime.close_and_wait();
    }

    #[test]

    fn cancel_callback_may_close_its_own_request() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let cancelled_request: Arc<Mutex<Option<crate::ResourceRequestHandle>>> =
            Arc::new(Mutex::new(None));
        let provider_request = Arc::clone(&cancelled_request);
        let closed = Arc::new(AtomicUsize::new(0));
        let callback_closed = Arc::clone(&closed);
        let (sender, receiver) = std::sync::mpsc::channel();
        commit_resource_provider(&runtime, move |request, handle| {
            if request.requested_url.as_deref() != Some("custom://cancel-style.json") {
                return ResourceProviderDecision::PassThrough;
            }
            let callback_request = Arc::clone(&provider_request);
            let closed = Arc::clone(&callback_closed);
            handle
                .set_cancel_callback(move || {
                    let request = callback_request.lock().unwrap().take();
                    request.expect("the cancelled request").close().unwrap();
                    closed.fetch_add(1, Ordering::SeqCst);
                })
                .unwrap();
            *provider_request.lock().unwrap() = Some(handle);
            sender.send(()).unwrap();
            ResourceProviderDecision::Handle
        });

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://cancel-style.json").unwrap();
        receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should handle the style request");

        map.close_and_wait();

        assert!(wait_for_condition(|| closed.load(Ordering::SeqCst) == 1));
        assert!(cancelled_request.lock().unwrap().is_none());
        runtime.close_and_wait();
    }

    #[test]

    fn cancel_callback_skips_a_completed_request() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let cancels = Arc::new(AtomicUsize::new(0));
        let callback_cancels = Arc::clone(&cancels);
        commit_resource_provider(&runtime, move |request, handle| {
            if request.requested_url.as_deref() != Some("custom://cancel-style.json") {
                return ResourceProviderDecision::PassThrough;
            }
            let cancels = Arc::clone(&callback_cancels);
            handle
                .set_cancel_callback(move || {
                    cancels.fetch_add(1, Ordering::SeqCst);
                })
                .unwrap();
            // Completing releases the handle, so no Rust view of the request
            // survives to query its cancellation afterwards; the cancel count
            // carries the negative instead.
            handle
                .complete(&crate::test_support::ok_response(
                    PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                ))
                .unwrap();
            ResourceProviderDecision::Handle
        });

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://cancel-style.json").unwrap();
        // The response reaches the style before the map goes away.
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));

        map.close_and_wait();

        // MapLibre runs its cancel hook on every request teardown, so the
        // retired map proves the completed request reported no cancellation.
        crate::completion::blocking(runtime.barrier());
        assert_eq!(cancels.load(Ordering::SeqCst), 0);
        runtime.close_and_wait();
    }

    #[test]

    fn cancel_registration_reports_an_already_cancelled_request() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        commit_resource_provider(&runtime, move |request, handle| {
            if request.requested_url.as_deref() != Some("custom://cancel-style.json") {
                return ResourceProviderDecision::PassThrough;
            }
            sender.send(handle).unwrap();
            ResourceProviderDecision::Handle
        });

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url("custom://cancel-style.json").unwrap();
        let handle = receiver
            .recv_timeout(Duration::from_secs(5))
            .expect("provider should send handled request");
        map.close_and_wait();
        assert!(wait_for_condition(|| handle.cancelled().unwrap()));

        let cancels = Arc::new(AtomicUsize::new(0));
        let callback_cancels = Arc::clone(&cancels);
        let already_cancelled = handle
            .set_cancel_callback(move || {
                callback_cancels.fetch_add(1, Ordering::SeqCst);
            })
            .unwrap();
        assert!(already_cancelled);
        assert_eq!(cancels.load(Ordering::SeqCst), 0);

        // The accepted registration remains consumed after reporting cancellation.
        assert_eq!(
            handle.set_cancel_callback(|| {}).unwrap_err().kind(),
            ErrorKind::InvalidState
        );
        handle.close().unwrap();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_provider_error_response_becomes_copied_loading_failure_event() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        runtime
            .set_resource_provider(crate::ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() == Some("custom://broken-style.json") {
                    handle
                        .complete(&crate::test_support::error_response(
                            ResourceErrorReason::Other,
                            "provider failed",
                        ))
                        .unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        let map_id = map.id();
        map.set_style_url("custom://broken-style.json").unwrap();

        let event = wait_for_map_loading_failure(&runtime);
        let copied_message = event.message.clone();
        // The copy stays intact after the drain that ends the batch's window.
        let _ = runtime.drain_events().unwrap().get().unwrap().events;

        assert_eq!(event.source, map_id.get());
        assert_eq!(event.r#type, RuntimeEventType::MapLoadingFailed);
        assert_eq!(event.message, copied_message);
        assert!(event.message.contains("provider failed"));

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_transform_installs_replaces_clears_and_releases_state() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);

        completion::blocking(
            runtime.set_resource_transform(resource_transform(move |request| {
                let _ = &first_callback;
                assert!(matches!(
                    request.kind,
                    ResourceKind::Style | ResourceKind::Unrecognized(_)
                ));
                None
            })),
        );
        assert_eq!(Arc::strong_count(&first), 2);

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        completion::blocking(runtime.set_resource_transform(resource_transform(move |_| {
            let _ = &second_callback;
            Some("https://example.test/replacement".to_owned())
        })));
        wait_for_arc_release(&first);
        assert_eq!(Arc::strong_count(&second), 2);

        completion::blocking(runtime.clear_resource_transform());
        wait_for_arc_release(&second);
        runtime.close_and_wait();
    }

    /// The browser has no in-process TCP server, so the runner serves the two
    /// style documents and answers 404 for everything else. That makes the
    /// server the oracle: a rewrite that did not happen fails the style load
    /// rather than fetching an identical document, and the layer each document
    /// carries names which one was used. See scripts/run-browser-test.mjs.
    #[cfg(target_os = "emscripten")]
    #[test]

    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let origin = std::env::var("MLN_FFI_TEST_FIXTURE_ORIGIN").expect(
            "MLN_FFI_TEST_FIXTURE_ORIGIN is unset; run the suite through \
             `mise run //bindings/rust:test emscripten-wasm32-webgl`",
        );
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let transform_url = format!("{origin}/__fixture/rewritten-style.json");
        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the layer id says so.
        runtime
            .set_resource_transform(resource_transform(move |request| {
                (request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json"))
                .then(|| transform_url.clone())
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url(&format!("{origin}/__fixture/original-style.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        // Contains rather than equals: MapLibre adds its own annotation layer to
        // every style it loads.
        let layer_ids = map.list_style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(layer_ids.take().unwrap().iter().any(|id| id == "rewritten"));

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{origin}/__fixture/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        let layer_ids = map.list_style_layer_ids().unwrap();
        assert!(layer_ids.wait(Duration::from_secs(5)).unwrap());
        assert!(
            layer_ids
                .take()
                .unwrap()
                .iter()
                .any(|id| id == "original-after-clear")
        );

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[cfg(not(target_os = "emscripten"))]
    #[test]

    fn resource_transform_rewrites_style_url_and_clear_restores_original_url() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_style_server(2);
        let transform_base_url = base_url.clone();

        // Matches the URL loaded after the clear as well, so a transform that
        // outlived the clear rewrites that request too and the recorded path
        // says so.
        runtime
            .set_resource_transform(resource_transform(move |request| {
                if request.url.ends_with("/original-style.json")
                    || request.url.ends_with("/original-after-clear.json")
                {
                    Some(format!("{transform_base_url}/rewritten-style.json"))
                } else {
                    None
                }
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url(&format!("{base_url}/original-style.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/rewritten-style.json"
        );

        runtime.clear_resource_transform().unwrap();
        map.set_style_url(&format!("{base_url}/original-after-clear.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            "/original-after-clear.json"
        );

        map.close_and_wait();
        runtime.close_and_wait();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]

    fn http_header_transform_reaches_requests_and_clear_stops_it() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let (base_url, requests, server) = spawn_recording_style_server(2);
        runtime
            .set_http_header_transform(http_header_transform(|request| {
                assert_eq!(request.kind, ResourceKind::Style);
                vec![("X-Map-Token".to_owned(), "secret".to_owned())]
            }))
            .unwrap();

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.set_style_url(&format!("{base_url}/with-header.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        let first = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            first
                .lines()
                .any(|line| line.eq_ignore_ascii_case("X-Map-Token: secret"))
        );

        runtime.clear_http_header_transform().unwrap();
        map.set_style_url(&format!("{base_url}/after-clear.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        let second = requests.recv_timeout(Duration::from_secs(5)).unwrap();
        assert!(
            !second
                .lines()
                .any(|line| line.to_ascii_lowercase().starts_with("x-map-token:"))
        );

        map.close_and_wait();
        runtime.close_and_wait();
        server.join().unwrap();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]

    fn http_header_transform_skips_non_http_urls() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        runtime
            .set_resource_transform(resource_transform(|_| None))
            .unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_http_header_transform(http_header_transform(move |_| {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                Vec::new()
            }))
            .unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        map.set_style_url("jar:file:/packaged/style.json").unwrap();
        let _ = wait_for_map_loading_failure(&runtime);
        assert_eq!(calls.load(Ordering::SeqCst), 0);

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    #[test]

    fn http_header_transform_preserves_same_origin_and_strips_cross_origin_redirects() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let (origin_url, requests, servers) = spawn_redirect_style_servers();
        runtime
            .set_http_header_transform(http_header_transform(|_| {
                vec![("X-Map-Token".to_owned(), "secret".to_owned())]
            }))
            .unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        map.set_style_url(&format!("{origin_url}/same-start.json"))
            .unwrap();
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
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
        assert!(wait_for_event(&runtime, RuntimeEventType::MapStyleLoaded));
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-start.json".to_owned(), true)
        );
        assert_eq!(
            requests.recv_timeout(Duration::from_secs(5)).unwrap(),
            ("/cross-final.json".to_owned(), false)
        );

        map.close_and_wait();
        runtime.close_and_wait();
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
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let error = runtime
            .set_http_header_transform(http_header_transform(|_| Vec::new()))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::Unsupported);
        runtime.close_and_wait();
    }

    #[test]

    fn resource_transform_replacement_after_map_creation_releases_previous_state() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        completion::blocking(runtime.set_resource_transform(resource_transform(move |_| {
            let _ = &first_callback;
            None
        })));
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        completion::blocking(runtime.set_resource_transform(resource_transform(move |_| {
            let _ = &second_callback;
            None
        })));

        wait_for_arc_release(&first);
        assert_eq!(Arc::strong_count(&second), 2);

        map.close_and_wait();
        runtime.close_and_wait();
        wait_for_arc_release(&second);
    }

    #[test]

    fn runtime_teardown_releases_resource_transform_state() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        completion::blocking(runtime.set_resource_transform(resource_transform(move |_| {
            let _ = &callback_token;
            None
        })));
        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close_and_wait();

        wait_for_arc_release(&token);
    }

    #[test]
    // Rust regression: documents the Rust binding's late transform-install
    // guard after map creation.
    fn resource_transform_installs_after_map_creation() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        runtime
            .set_resource_transform(resource_transform(|_| None))
            .unwrap();

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn resource_transform_clears_after_map_was_closed_and_releases_state() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(resource_transform(move |_| {
                let _ = &callback_token;
                None
            }))
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        map.close_and_wait();

        completion::blocking(runtime.clear_resource_transform());
        wait_for_arc_release(&token);

        runtime.close_and_wait();
    }

    #[test]

    fn a_drain_reports_map_events_in_queue_order_and_copies_outlive_the_drain() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));
        let map_id = map.id();

        let command = map.set_style_json(b"{").unwrap();
        assert!(command.wait(Duration::from_secs(5)).unwrap());
        let completion = command.take().unwrap();
        assert_eq!(completion.disposition, crate::CommandDisposition::Failed);
        assert_eq!(completion.raw_status, sys::MLN_STATUS_NATIVE_ERROR);
        assert!(!completion.diagnostic.is_empty());

        let events = runtime.drain_events().unwrap().get().unwrap().events;
        let types = events.iter().map(|event| event.r#type).collect::<Vec<_>>();
        assert!(
            !types.is_empty(),
            "a failed style load should queue an event, got {types:?}"
        );
        let owned = events
            .into_iter()
            .find(|event| event.r#type == RuntimeEventType::MapLoadingFailed)
            .expect("a malformed style should queue a loading-failed event");
        assert_eq!(owned.source, map_id.get());

        // A later drain leaves the copied event readable.
        assert!(
            runtime
                .drain_events()
                .unwrap()
                .get()
                .unwrap()
                .events
                .is_empty()
        );
        assert_eq!(owned.source, map_id.get());
        assert_eq!(owned.r#type, RuntimeEventType::MapLoadingFailed);
        assert!(!owned.message.is_empty());

        map.close_and_wait();
        runtime.close_and_wait();
    }

    #[test]

    fn runtime_close_with_live_map_is_rust_invalid_state_and_retryable() {
        let runtime = crate::runtime_create(&crate::RuntimeOptions::default()).unwrap();
        let map = crate::completion::blocking(runtime.map_create(&MapOptions::default()));

        let error = runtime.release().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_STATE));

        map.close_and_wait();
        runtime.close_and_wait();
    }
}
