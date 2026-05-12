use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::fmt;
use std::rc::{Rc, Weak};

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::events::{MapId, RuntimeEvent, RuntimeEventSource, empty_runtime_event};
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::MapState;
use crate::resource::{
    ResourceProviderState, ResourceTransformState, noop_resource_transform_descriptor,
};
use crate::{Error, ErrorKind, MapHandle, MapOptions, ResourceProviderDecision, Result};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ThreadAffineNativeHandle<sys::mln_runtime>,
    next_map_id: Cell<u64>,
    has_created_map: Cell<bool>,
    map_ids: RefCell<HashMap<usize, MapId>>,
    map_states: RefCell<HashMap<usize, Weak<MapState>>>,
    resource_transform: RefCell<Option<Box<ResourceTransformState>>>,
    resource_provider: RefCell<Option<Box<ResourceProviderState>>>,
}

impl RuntimeState {
    fn new(ptr: std::ptr::NonNull<sys::mln_runtime>) -> Self {
        // SAFETY: ptr came from successful mln_runtime_create and is paired
        // with the matching runtime destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_raw(ptr, sys::mln_runtime_destroy, "mln_runtime")
        };
        Self {
            handle,
            next_map_id: Cell::new(1),
            has_created_map: Cell::new(false),
            map_ids: RefCell::new(HashMap::new()),
            map_states: RefCell::new(HashMap::new()),
            resource_transform: RefCell::new(None),
            resource_provider: RefCell::new(None),
        }
    }

    pub(crate) fn as_ptr(&self) -> Result<*mut sys::mln_runtime> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("RuntimeHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close()?;
        self.resource_transform.borrow_mut().take();
        self.resource_provider.borrow_mut().take();
        Ok(())
    }

    fn set_resource_provider<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let replacement = ResourceProviderState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live. descriptor contains a C trampoline and a
        // user_data pointer to replacement, which remains alive on success. On
        // failure, native preserves the previous provider and replacement is
        // dropped below.
        support::check(unsafe { sys::mln_runtime_set_resource_provider(runtime, &descriptor) })?;
        self.resource_provider.borrow_mut().replace(replacement);
        Ok(())
    }

    fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let replacement = ResourceTransformState::new(callback);
        let descriptor = replacement.descriptor();

        // SAFETY: runtime is live. descriptor contains a C trampoline and a
        // user_data pointer to replacement, which remains alive on success. On
        // failure, native preserves the previous transform and replacement is
        // dropped below.
        support::check(unsafe { sys::mln_runtime_set_resource_transform(runtime, &descriptor) })?;
        self.resource_transform.borrow_mut().replace(replacement);
        Ok(())
    }

    fn clear_resource_transform(&self) -> Result<()> {
        self.check_resource_callbacks_allowed()?;
        let runtime = self.as_ptr()?;
        let descriptor = noop_resource_transform_descriptor();

        // SAFETY: runtime is live. The C ABI has no null clear operation, so a
        // no-op transform with static function state restores pass-through
        // behavior without retaining Rust callback state.
        support::check(unsafe { sys::mln_runtime_set_resource_transform(runtime, &descriptor) })?;
        self.resource_transform.borrow_mut().take();
        Ok(())
    }

    fn check_resource_callbacks_allowed(&self) -> Result<()> {
        if self.has_created_map.get() {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "resource callbacks must be configured before creating maps from the runtime",
            ));
        }
        Ok(())
    }

    pub(crate) fn register_map(&self, ptr: *mut sys::mln_map) -> MapId {
        self.has_created_map.set(true);
        let id = MapId::new(self.next_map_id.get());
        self.next_map_id.set(id.get().saturating_add(1));
        self.map_ids.borrow_mut().insert(ptr as usize, id);
        id
    }

    pub(crate) fn register_map_state(&self, ptr: *mut sys::mln_map, state: Weak<MapState>) {
        if !ptr.is_null() {
            self.map_states.borrow_mut().insert(ptr as usize, state);
        }
    }

    pub(crate) fn unregister_map(&self, ptr: *mut sys::mln_map) {
        if !ptr.is_null() {
            self.map_ids.borrow_mut().remove(&(ptr as usize));
            self.map_states.borrow_mut().remove(&(ptr as usize));
        }
    }

    fn apply_event_side_effects(&self, raw: &sys::mln_runtime_event) {
        if raw.source_type != sys::MLN_RUNTIME_EVENT_SOURCE_MAP {
            return;
        }
        let state = self
            .map_states
            .borrow()
            .get(&(raw.source as usize))
            .and_then(Weak::upgrade);
        let Some(state) = state else {
            return;
        };
        match raw.type_ {
            sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED => {
                state.finish_custom_geometry_sources_pending_url_cleanup();
            }
            sys::MLN_RUNTIME_EVENT_MAP_LOADING_FAILED => {
                state.cancel_custom_geometry_sources_pending_url_cleanup();
            }
            _ => {}
        }
    }

    #[cfg(test)]
    pub(crate) fn apply_event_side_effects_for_testing(&self, raw: &sys::mln_runtime_event) {
        self.apply_event_side_effects(raw);
    }

    fn source_for_event(&self, raw: &sys::mln_runtime_event) -> RuntimeEventSource {
        match raw.source_type {
            sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME => RuntimeEventSource::Runtime,
            sys::MLN_RUNTIME_EVENT_SOURCE_MAP => self
                .map_ids
                .borrow()
                .get(&(raw.source as usize))
                .copied()
                .map(RuntimeEventSource::Map)
                .unwrap_or(RuntimeEventSource::UnknownMap),
            source_type => RuntimeEventSource::Unknown(source_type),
        }
    }
}

/// Owner-thread runtime handle for MapLibre Native work and event polling.
pub struct RuntimeHandle {
    pub(crate) inner: Rc<RuntimeState>,
}

impl fmt::Debug for RuntimeHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RuntimeHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl RuntimeHandle {
    /// Creates a runtime on the current thread using native default options.
    pub fn new() -> Result<Self> {
        support::validate_abi_version()?;

        let mut out = support::ptr::OutPtr::<sys::mln_runtime>::new();
        // SAFETY: Passing null options requests native defaults. out is a valid
        // null-initialized out-pointer owned by this call.
        support::check(unsafe { sys::mln_runtime_create(std::ptr::null(), out.as_mut_ptr()) })?;
        let ptr = out_handle(out, "mln_runtime")?;

        Ok(Self {
            inner: Rc::new(RuntimeState::new(ptr)),
        })
    }

    /// Creates a map owned by this runtime with native default map options.
    pub fn create_map(&self) -> Result<MapHandle> {
        MapHandle::new(self)
    }

    /// Creates a map owned by this runtime with explicit map options.
    pub fn create_map_with_options(&self, options: &MapOptions) -> Result<MapHandle> {
        MapHandle::with_options(self, options)
    }

    /// Installs or replaces the runtime-scoped network resource provider.
    ///
    /// The provider must be installed before creating maps from this runtime.
    /// Native code may invoke it from worker or network threads, so the closure
    /// must be thread-safe and `'static`. Keep the closure quick, and do not
    /// call map or runtime APIs from it. Return `PassThrough` to let native
    /// networking handle the request. Return `Handle` to complete or release
    /// the provided `ResourceRequestHandle` inline or later. If the callback
    /// completes the handle inline, the wrapper returns native `Handle` even
    /// when the closure returns `PassThrough`, preventing native double
    /// handling.
    pub fn set_resource_provider<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceRequest, crate::ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        self.inner.set_resource_provider(callback)
    }

    /// Installs or replaces the runtime-scoped network URL transform.
    ///
    /// The transform must be installed before creating maps from this runtime.
    /// Native code may invoke it from worker or network threads, so the closure
    /// must be thread-safe and `'static`. Keep the closure quick, and do not
    /// call MapLibre Native APIs from it. Returning `Some(url)` replaces the
    /// request URL; returning `None` or an empty string keeps the original URL.
    /// Panics are contained and treated by native code as no rewrite.
    pub fn set_resource_transform<F>(&self, callback: F) -> Result<()>
    where
        F: Fn(crate::ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        self.inner.set_resource_transform(callback)
    }

    /// Clears the runtime-scoped network URL transform.
    ///
    /// Like installation, clearing must happen before creating maps from this
    /// runtime. The current C ABI has no null clear operation. This method
    /// installs a native no-op transform before releasing Rust callback state,
    /// restoring pass-through URL behavior while honoring native install
    /// constraints.
    pub fn clear_resource_transform(&self) -> Result<()> {
        self.inner.clear_resource_transform()
    }

    /// Runs one pending owner-thread task for this runtime.
    pub fn run_once(&self) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is a live runtime handle owned by this wrapper.
        support::check(unsafe { sys::mln_runtime_run_once(runtime) })
    }

    /// Polls one queued runtime event and copies it into an owned Rust value.
    pub fn poll_event(&self) -> Result<Option<RuntimeEvent>> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
        support::check(unsafe {
            sys::mln_runtime_poll_event(runtime, &mut event, &mut has_event)
        })?;
        if !has_event {
            return Ok(None);
        }

        let raw_event = event;
        let source = self.inner.source_for_event(&raw_event);
        let event = RuntimeEvent::from_native(&raw_event, source)?;
        self.inner.apply_event_side_effects(&raw_event);
        Ok(Some(event))
    }

    /// Polls and discards one queued runtime event, returning whether one was present.
    pub fn discard_one_event(&self) -> Result<bool> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
        // The event is intentionally not decoded because this method only
        // drains native storage.
        support::check(unsafe {
            sys::mln_runtime_poll_event(runtime, &mut event, &mut has_event)
        })?;
        if has_event {
            self.inner.apply_event_side_effects(&event);
        }
        Ok(has_event)
    }

    /// Polls and discards queued runtime events until the queue is empty.
    pub fn drain_events(&self) -> Result<usize> {
        let mut count = 0;
        while self.discard_one_event()? {
            count += 1;
        }
        Ok(count)
    }

    /// Explicitly destroys the runtime.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so child
    /// handles that retain the runtime can still close safely.
    pub fn close(&self) -> Result<()> {
        if self.inner.is_closed() {
            return Ok(());
        }
        if Rc::strong_count(&self.inner) > 1 {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "RuntimeHandle cannot close while child handles are live",
            ));
        }
        self.inner.close()
    }
}

#[cfg(test)]
mod tests {
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::time::Duration;

    use super::*;
    use crate::{
        ErrorKind, ResourceKind, ResourceProviderDecision, ResourceResponse, RuntimeEventSource,
        RuntimeEventType,
    };

    const PROVIDER_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;

    fn wait_for_runtime_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
        for _ in 0..100 {
            let _ = runtime.run_once();
            while let Ok(Some(event)) = runtime.poll_event() {
                if event.event_type == event_type {
                    return true;
                }
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        false
    }

    #[test]
    fn runtime_create_run_poll_drain_and_close() {
        let runtime = RuntimeHandle::new().unwrap();

        runtime.run_once().unwrap();
        let _ = runtime.poll_event().unwrap();
        let _ = runtime.discard_one_event().unwrap();
        runtime.drain_events().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn runtime_close_is_idempotent() {
        let runtime = RuntimeHandle::new().unwrap();

        runtime.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_installs_replaces_and_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
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
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
    }

    #[test]
    fn resource_provider_replacement_rolls_back_when_native_install_fails() {
        let runtime = RuntimeHandle::new().unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_provider(move |_, _| {
                let _ = &first_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap();
        let map = runtime.create_map().unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        let error = runtime
            .set_resource_provider(move |_, _| {
                let _ = &second_callback;
                crate::ResourceProviderDecision::PassThrough
            })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 1);

        map.close().unwrap();
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&first), 1);
    }

    #[test]
    fn resource_provider_rejects_install_after_map_was_closed() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        map.close().unwrap();

        let error = runtime
            .set_resource_provider(|_, _| ResourceProviderDecision::PassThrough)
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_completes_style_request_inline_through_c_abi() {
        let runtime = RuntimeHandle::new().unwrap();
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        runtime
            .set_resource_provider(move |request, handle| {
                if request.url != "custom://style.json" {
                    return ResourceProviderDecision::PassThrough;
                }
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(ResourceResponse::ok(
                        PROVIDER_STYLE_JSON.as_bytes().to_vec(),
                    ))
                    .unwrap();
                assert!(handle.complete(ResourceResponse::no_content()).is_err());
                assert!(handle.is_cancelled().is_err());
                ResourceProviderDecision::PassThrough
            })
            .unwrap();

        let map = runtime.create_map().unwrap();
        map.set_style_url("custom://style.json").unwrap();

        assert!(wait_for_runtime_event(
            &runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        assert_eq!(calls.load(Ordering::SeqCst), 1);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_provider_completes_style_request_from_another_thread() {
        let runtime = RuntimeHandle::new().unwrap();
        let (sender, receiver) = std::sync::mpsc::channel();
        runtime
            .set_resource_provider(move |request, handle| {
                if request.url == "custom://async-style.json" {
                    sender.send(handle).unwrap();
                    ResourceProviderDecision::Handle
                } else {
                    ResourceProviderDecision::PassThrough
                }
            })
            .unwrap();

        let map = runtime.create_map().unwrap();
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
            assert!(handle.complete(ResourceResponse::no_content()).is_err());
            handle.close();
        })
        .join()
        .unwrap();

        assert!(wait_for_runtime_event(
            &runtime,
            RuntimeEventType::MapStyleLoaded
        ));
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_installs_replaces_clears_and_releases_state() {
        let runtime = RuntimeHandle::new().unwrap();
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
        assert_eq!(Arc::strong_count(&first), 1);
        assert_eq!(Arc::strong_count(&second), 2);

        runtime.clear_resource_transform().unwrap();
        assert_eq!(Arc::strong_count(&second), 1);
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_replacement_rolls_back_when_native_install_fails() {
        let runtime = RuntimeHandle::new().unwrap();
        let first = Arc::new(());
        let first_callback = Arc::clone(&first);
        runtime
            .set_resource_transform(move |_| {
                let _ = &first_callback;
                None
            })
            .unwrap();
        let map = runtime.create_map().unwrap();

        let second = Arc::new(());
        let second_callback = Arc::clone(&second);
        let error = runtime
            .set_resource_transform(move |_| {
                let _ = &second_callback;
                None
            })
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(Arc::strong_count(&first), 2);
        assert_eq!(Arc::strong_count(&second), 1);

        map.close().unwrap();
        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&first), 1);
    }

    #[test]
    fn runtime_teardown_releases_resource_transform_state() {
        let runtime = RuntimeHandle::new().unwrap();
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

        assert_eq!(Arc::strong_count(&token), 1);
    }

    #[test]
    fn resource_transform_rejects_install_after_map_creation() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        let error = runtime.set_resource_transform(|_| None).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_rejects_set_after_map_was_closed() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        map.close().unwrap();

        let error = runtime.set_resource_transform(|_| None).unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        runtime.close().unwrap();
    }

    #[test]
    fn resource_transform_rejects_clear_after_map_was_closed_and_keeps_state_until_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        runtime
            .set_resource_transform(move |_| {
                let _ = &callback_token;
                None
            })
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        let map = runtime.create_map().unwrap();
        map.close().unwrap();

        let error = runtime.clear_resource_transform().unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);
        assert_eq!(Arc::strong_count(&token), 2);

        runtime.close().unwrap();
        assert_eq!(Arc::strong_count(&token), 1);
    }

    #[test]
    fn poll_event_returns_none_for_empty_queue() {
        let runtime = RuntimeHandle::new().unwrap();

        assert_eq!(runtime.poll_event().unwrap(), None);

        runtime.close().unwrap();
    }

    #[test]
    fn poll_event_returns_owned_map_event_and_source_id() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();
        let map_id = map.id();

        let error = map.set_style_json("{").unwrap_err();
        assert!(matches!(
            error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::NativeError
        ));

        let mut loading_failed = None;
        for _ in 0..8 {
            let Some(event) = runtime.poll_event().unwrap() else {
                break;
            };
            if event.event_type == RuntimeEventType::MapLoadingFailed {
                loading_failed = Some(event);
                break;
            }
        }
        let event = loading_failed.expect("malformed style should enqueue loading-failed event");
        let copied_message = event.message.clone();

        let _ = runtime.poll_event().unwrap();

        assert_eq!(event.source, RuntimeEventSource::Map(map_id));
        assert_eq!(event.event_type, RuntimeEventType::MapLoadingFailed);
        assert_eq!(event.message, copied_message);
        assert!(
            event
                .message
                .as_deref()
                .is_some_and(|message| !message.is_empty())
        );

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn runtime_close_with_live_map_is_rust_invalid_state_and_retryable() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = runtime.create_map().unwrap();

        let error = runtime.close().unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(error.raw_status(), None);

        runtime.run_once().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }
}
