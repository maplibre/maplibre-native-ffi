use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::fmt;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::events::{MapId, RuntimeEvent, RuntimeEventSource, empty_runtime_event};
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::{Error, ErrorKind, MapHandle, MapOptions, Result};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ThreadAffineNativeHandle<sys::mln_runtime>,
    next_map_id: Cell<u64>,
    map_ids: RefCell<HashMap<usize, MapId>>,
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
            map_ids: RefCell::new(HashMap::new()),
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
        self.handle.close()
    }

    pub(crate) fn register_map(&self, ptr: *mut sys::mln_map) -> MapId {
        let id = MapId::new(self.next_map_id.get());
        self.next_map_id.set(id.get().saturating_add(1));
        self.map_ids.borrow_mut().insert(ptr as usize, id);
        id
    }

    pub(crate) fn unregister_map(&self, ptr: *mut sys::mln_map) {
        if !ptr.is_null() {
            self.map_ids.borrow_mut().remove(&(ptr as usize));
        }
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

        let source = self.inner.source_for_event(&event);
        RuntimeEvent::from_native(&event, source).map(Some)
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
    use super::*;
    use crate::{ErrorKind, RuntimeEventSource, RuntimeEventType};

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
