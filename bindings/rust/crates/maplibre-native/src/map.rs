use std::cell::RefCell;
use std::fmt;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::handle::{ThreadAffineNativeHandle, out_handle};
use crate::runtime::{RuntimeHandle, RuntimeState};
use crate::{MapOptions, Result};

#[derive(Debug)]
pub(crate) struct MapState {
    handle: ThreadAffineNativeHandle<sys::mln_map>,
    runtime: RefCell<Option<Rc<RuntimeState>>>,
}

impl MapState {
    fn new(ptr: std::ptr::NonNull<sys::mln_map>, runtime: Rc<RuntimeState>) -> Self {
        // SAFETY: ptr came from successful mln_map_create and is paired with
        // the matching map destroy function.
        let handle =
            unsafe { ThreadAffineNativeHandle::from_raw(ptr, sys::mln_map_destroy, "mln_map") };
        Self {
            handle,
            runtime: RefCell::new(Some(runtime)),
        }
    }

    fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    fn close(&self) -> Result<()> {
        self.handle.close()?;
        self.runtime.borrow_mut().take();
        Ok(())
    }
}

/// Owner-thread map handle bound to a retained runtime.
pub struct MapHandle {
    pub(crate) inner: Rc<MapState>,
}

impl fmt::Debug for MapHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MapHandle")
            .field("closed", &self.inner.is_closed())
            .finish()
    }
}

impl MapHandle {
    /// Creates a map with native default map options on the runtime owner thread.
    pub fn new(runtime: &RuntimeHandle) -> Result<Self> {
        Self::with_options(runtime, &MapOptions::default())
    }

    /// Creates a map with explicit map options on the runtime owner thread.
    pub fn with_options(runtime: &RuntimeHandle, options: &MapOptions) -> Result<Self> {
        let runtime_ptr = runtime.inner.as_ptr()?;
        let mut out = support::ptr::OutPtr::<sys::mln_map>::new();
        let raw_options = options.to_native();

        // SAFETY: runtime_ptr is a live runtime handle. raw_options is a
        // materialized map descriptor with size filled by the binding. out is a
        // valid null-initialized out-pointer owned by this call.
        support::check(unsafe {
            sys::mln_map_create(runtime_ptr, &raw_options, out.as_mut_ptr())
        })?;
        let ptr = out_handle(out, "mln_map")?;

        Ok(Self {
            inner: Rc::new(MapState::new(ptr, Rc::clone(&runtime.inner))),
        })
    }

    /// Explicitly destroys the map.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live in the shared state so future child
    /// handles can continue to retain and close the map safely.
    pub fn close(&self) -> Result<()> {
        self.inner.close()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn map_create_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_create_with_options_and_close() {
        let runtime = RuntimeHandle::new().unwrap();
        let options = MapOptions::new(320, 240, 2.0).with_mode(crate::MapMode::Static);
        let map = MapHandle::with_options(&runtime, &options).unwrap();

        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_close_is_idempotent() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        map.close().unwrap();
        map.close().unwrap();
        runtime.close().unwrap();
    }

    #[test]
    fn map_retains_runtime_after_runtime_handle_is_dropped() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::new(&runtime).unwrap();

        drop(runtime);

        map.close().unwrap();
    }
}
