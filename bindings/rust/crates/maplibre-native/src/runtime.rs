use std::fmt;
use std::mem;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::{Error, ErrorKind, MapHandle, Result};

#[derive(Debug)]
pub(crate) struct RuntimeState {
    handle: ThreadAffineNativeHandle<sys::mln_runtime>,
}

impl RuntimeState {
    fn new(ptr: std::ptr::NonNull<sys::mln_runtime>) -> Self {
        // SAFETY: ptr came from successful mln_runtime_create and is paired
        // with the matching runtime destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_raw(ptr, sys::mln_runtime_destroy, "mln_runtime")
        };
        Self { handle }
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

    /// Creates a map owned by this runtime.
    pub fn create_map(&self) -> Result<MapHandle> {
        MapHandle::new(self)
    }

    /// Runs one pending owner-thread task for this runtime.
    pub fn run_once(&self) -> Result<()> {
        let runtime = self.inner.as_ptr()?;
        // SAFETY: runtime is a live runtime handle owned by this wrapper.
        support::check(unsafe { sys::mln_runtime_run_once(runtime) })
    }

    /// Polls and discards one queued runtime event, returning whether one was present.
    ///
    /// Owned event values are added in a later milestone. This explicitly named
    /// discard primitive avoids exposing borrowed native event storage.
    pub fn discard_one_event(&self) -> Result<bool> {
        let runtime = self.inner.as_ptr()?;
        let mut event = empty_runtime_event();
        let mut has_event = false;

        // SAFETY: runtime is live, event points to initialized writable storage
        // with a valid size field, and has_event points to writable bool storage.
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

fn empty_runtime_event() -> sys::mln_runtime_event {
    sys::mln_runtime_event {
        size: mem::size_of::<sys::mln_runtime_event>() as u32,
        type_: 0,
        source_type: sys::MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        source: std::ptr::null_mut(),
        code: 0,
        payload_type: sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE,
        payload: std::ptr::null(),
        payload_size: 0,
        message: std::ptr::null(),
        message_size: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn runtime_create_run_poll_drain_and_close() {
        let runtime = RuntimeHandle::new().unwrap();

        runtime.run_once().unwrap();
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
